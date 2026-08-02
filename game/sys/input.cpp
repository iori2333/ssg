///
/// Cross-platform input
///

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include "input.h"

#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_stdinc.h"
#include "util/enum_flags.h"
#include "util/guard.h"

// MSVC's static analyzer suggests to make the functions below `constexpr`,
// which won't work because they are used in other translation units and this
// is not a header.
#pragma warning(suppress : 26497) // f.4
bool InputIsOk(InputBits key) {
  return ((key == KeyReturn) || (key == KeyTama));
}

#pragma warning(suppress : 26497) // f.4
bool InputIsCancel(InputBits key) {
  return ((key == KeyBomb) || (key == KeyEscape));
}

int_fast8_t InputOptionKeyDelta(InputBits key) {
  if (InputIsOk(key) || (key == KeyRight)) {
    return 1;
  }
  if (key == KeyLeft) {
    return -1;
  }
  return 0;
}

///
/// Input via SDL events
///

// Do scancodes and key modes still fit into 16 bits each?
static_assert(SDL_SCANCODE_COUNT <= std::numeric_limits<uint16_t>::max());
static_assert(SDL_KMOD_SCROLL <= std::numeric_limits<uint16_t>::max());

namespace {

enum class KeyMod : uint8_t {
  None = 0x00,
  LeftAlt = 0x01,
};

} // namespace

namespace util {
template <> inline constexpr bool EnableEnumFlags<KeyMod> = true;
} // namespace util

namespace {

// In-game scancode, received from SDL.
struct KeyScancode {
  uint16_t scancode;
  KeyMod mod;
};

// Static key binding.
struct KeyBind {
  uint16_t scancode;

  // Modifiers that *must* match.
  KeyMod mod_must = KeyMod::None;

  // Modifiers that *must not* match. Should contain all modifiers of other
  // bindings that share the same [scancode] to allow the matching to work
  // independently of the order within the binding array.
  KeyMod mod_filter = KeyMod::None;

  constexpr KeyBind(SDL_Scancode scancode, KeyMod mod_must = KeyMod::None,
                    KeyMod mod_filter = KeyMod::None)
      : scancode(scancode), mod_must(mod_must), mod_filter(mod_filter) {}

  [[nodiscard]] bool Matches(const KeyScancode &sc) const {
    return ((scancode == sc.scancode) && !(sc.mod & mod_filter) &&
            ((mod_must & sc.mod) == mod_must));
  }
};

constexpr std::array<std::pair<KeyBind, InputBits>, 23> KeyBindings = {{
    {SDL_SCANCODE_UP, KeyUp},
    {SDL_SCANCODE_DOWN, KeyDown},
    {SDL_SCANCODE_LEFT, KeyLeft},
    {SDL_SCANCODE_RIGHT, KeyRight},
    {SDL_SCANCODE_KP_1, KeyDownLeft},
    {SDL_SCANCODE_KP_2, KeyDown},
    {SDL_SCANCODE_KP_3, KeyDownRight},
    {SDL_SCANCODE_KP_4, KeyLeft},
    {SDL_SCANCODE_KP_6, KeyRight},
    {SDL_SCANCODE_KP_7, KeyUpLeft},
    {SDL_SCANCODE_KP_8, KeyUp},
    {SDL_SCANCODE_KP_9, KeyUpRight},
    {SDL_SCANCODE_Z, KeyTama},
    {SDL_SCANCODE_X, KeyBomb},
    {SDL_SCANCODE_LSHIFT, KeyShift},
    {SDL_SCANCODE_ESCAPE, KeyEscape},
    {SDL_SCANCODE_1, KeyStage1},
    {SDL_SCANCODE_2, KeyStage2},
    {SDL_SCANCODE_3, KeyStage3},
    {SDL_SCANCODE_4, KeyStage4},
    {SDL_SCANCODE_5, KeyStage5},
    {SDL_SCANCODE_6, KeyStage6},
    {{SDL_SCANCODE_RETURN, KeyMod::None, KeyMod::LeftAlt}, KeyReturn},
}};
constexpr std::array<std::pair<KeyBind, InputSystemBits>, 6>
    SystemKeyBindings = {{
        {SDL_SCANCODE_P, SystemKeySnapshot},
        {SDL_SCANCODE_LCTRL, SystemKeySkip},
        {SDL_SCANCODE_RCTRL, SystemKeySkip},
        {SDL_SCANCODE_F, SystemKeyBgmFade},
        {SDL_SCANCODE_D, SystemKeyBgmDevice},
        {SDL_SCANCODE_R, SystemKeyDemoRecord},
    }};

template <typename Bits>
void FlipKey(Bits &key_data, const auto &key_or_jbutton, Bits bits) {
  if (key_or_jbutton.down) {
    key_data |= bits;
  } else {
    key_data &= ~bits;
  }
}

struct Joypad {
  SDL_Joystick *joystick;
  uint8_t axis_x;
  uint8_t axis_y;
  InputPadButton button_pressed_last = 0;
  uint8_t buttons_held = 0;
};

} // namespace

struct InputSystem::Impl {
  InputSnapshot current;
  InputBits keyboard = 0;
  std::vector<Joypad> pads;
  std::vector<InputPadBinding> pad_bindings;
};

// SDL provides two abstractions for joypads:
//
// - SDL_Joystick, which uses numbered axes or buttons with no semantic
//   meaning, but works with every joypad ever
// - SDL_Gamepad, which provides a standardized interface for the typical kind
//   of modern gamepad with two analog sticks, a D-pad, and at least 4 face and
//   2 shoulder buttons, but doesn't work with joypads that don't match this
//   standard
//
// For buttons, SDL_Joystick's numbered IDs are exactly what we want. WinMM
// does the same, and the Joy Pad menu is designed around that. For axes,
// however, there is no UI, and SDL_Joystick provides no way of identifying
// which axis IDs correspond to a joypad's "main" X/Y axis, so we can only
// guess. Luckily, all joypads we've tested map their main X axis to ID 0
// and their main Y axis to ID 1.
//
// But we can still *try* opening a joypad via SDL_Gamepad. If that succeeds,
// we can correct this initial guess with the actual SDL_Joystick axis IDs
// (= the "bind") for the standard X and Y axes:
//
// 	https://discourse.libsdl.org/t/difference-between-joysticks-and-game-controllers/24028/2
namespace {

std::tuple<decltype(Joypad::axis_x), decltype(Joypad::axis_y)>
GetPadAxisIds(int device_index) {
  decltype(Joypad::axis_x) axis_x = 0;
  decltype(Joypad::axis_y) axis_y = 1;

  if (!SDL_IsGamepad(device_index)) {
    return {axis_x, axis_y};
  }
  auto *gamepad = SDL_OpenGamepad(device_index);
  if (gamepad == nullptr) {
    return {axis_x, axis_y};
  }
  auto gamepad_guard = util::MakeGuard(gamepad, SDL_CloseGamepad);

  int binding_count = 0;
  const std::unique_ptr<void, decltype(&SDL_free)> bindings_guard(
      static_cast<void *>(SDL_GetGamepadBindings(gamepad, &binding_count)),
      SDL_free);
  if (!bindings_guard) {
    return {axis_x, axis_y};
  }
  auto *const *bindings =
      static_cast<SDL_GamepadBinding **>(bindings_guard.get());

  // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) - SDL public API.
  for (const auto i : std::views::iota(0, binding_count)) {
    const auto *binding = bindings[i];
    if ((binding->input_type == SDL_GAMEPAD_BINDTYPE_AXIS) &&
        (binding->output_type == SDL_GAMEPAD_BINDTYPE_AXIS)) {
      if (binding->output.axis.axis == SDL_GAMEPAD_AXIS_LEFTX) {
        axis_x = binding->input.axis.axis;
      } else if (binding->output.axis.axis == SDL_GAMEPAD_AXIS_LEFTY) {
        axis_y = binding->input.axis.axis;
      }
    }
  }
  // NOLINTEND(cppcoreguidelines-pro-type-union-access)
  return {axis_x, axis_y};
}

std::vector<Joypad>::iterator FindPad(std::vector<Joypad> &pads,
                                      SDL_JoystickID id) {
  auto *joystick = SDL_GetJoystickFromID(id);
  const auto it = std::ranges::find_if(
      pads, [joystick](const auto &pad) { return (pad.joystick == joystick); });
  assert(it != pads.end());
  return it;
}

} // namespace

InputSystem::InputSystem() : impl_(std::make_unique<Impl>()) {}

InputSystem::~InputSystem() {
  for (auto &pad : impl_->pads) {
    SDL_CloseJoystick(pad.joystick);
  }
}

void InputSystem::SetPadBindings(std::span<const InputPadBinding> bindings) {
  impl_->pad_bindings.assign(bindings.begin(), bindings.end());
}

InputSnapshot InputSystem::Poll() {
  auto &state = *impl_;
  SDL_Event event;
  while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_KEY_DOWN,
                        SDL_EVENT_JOYSTICK_REMOVED) == 1) {
    switch (event.type) {
    case SDL_EVENT_JOYSTICK_AXIS_MOTION: {
      auto &pad = *FindPad(state.pads, event.jaxis.which);

      // The original WinMM backend did this without even taking the
      // range reported by joyGetDevCaps() into account. However, that
      // function returns a range of [0, 65535] with all joypads I've
      // tried, which matches SDL's fixed [-32768, 32767] range, so we
      // can do it here as well.
      const auto v = (event.jaxis.value >> 11);
      if (event.jaxis.axis == pad.axis_x) {
        state.current.pad &= ~(KeyLeft | KeyRight);
        InputBits axis_pad = 0;
        if (v <= -4) {
          axis_pad = KeyLeft;
        } else if (v >= 4) {
          axis_pad = KeyRight;
        }
        state.current.pad |= axis_pad;
      } else if (event.jaxis.axis == pad.axis_y) {
        state.current.pad &= ~(KeyUp | KeyDown);
        InputBits axis_pad = 0;
        if (v <= -4) {
          axis_pad = KeyUp;
        } else if (v >= 4) {
          axis_pad = KeyDown;
        }
        state.current.pad |= axis_pad;
      }
      break;
    }

    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP: {
      // SDL's numbering starts at 0.
      const InputPadButton id = (event.jbutton.button + 1);
      for (const auto &binding : state.pad_bindings) {
        if (id == binding.first) {
          FlipKey(state.current.pad, event.jbutton, binding.second);
        }
      }

      auto &pad = *FindPad(state.pads, event.jbutton.which);
      using Held = std::numeric_limits<decltype(pad.button_pressed_last)>;
      if (event.jbutton.down) {
        if (pad.buttons_held < Held::max()) {
          pad.buttons_held++;
        }
        pad.button_pressed_last = id;
      } else {
        if (pad.buttons_held > Held::min()) {
          pad.buttons_held--;
        }
      }
      break;
    }

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      const auto mod = SDL_GetModState();

      const KeyScancode scancode = {
          .scancode = static_cast<uint16_t>(event.key.scancode),
          .mod =
              (((mod & SDL_KMOD_LALT) != 0U) ? KeyMod::LeftAlt : KeyMod::None),
      };
      for (const auto &binding : KeyBindings) {
        if (binding.first.Matches(scancode)) {
          FlipKey(state.keyboard, event.key, binding.second);
        }
      }
      for (const auto &binding : SystemKeyBindings) {
        if (binding.first.Matches(scancode)) {
          FlipKey(state.current.system, event.key, binding.second);
        }
      }
      break;
    }

    case SDL_EVENT_JOYSTICK_REMOVED: {
      const auto pad = FindPad(state.pads, event.jdevice.which);
      SDL_CloseJoystick(pad->joystick);
      if (pad != (state.pads.end() - 1)) {
        *pad = state.pads.back();
      }
      state.pads.pop_back();
      break;
    }

    case SDL_EVENT_JOYSTICK_ADDED: {
      auto *joystick = SDL_OpenJoystick(event.jdevice.which);
      if (joystick == nullptr) {
        break;
      }
      const auto [axis_x, axis_y] = GetPadAxisIds(event.jdevice.which);
      state.pads.emplace_back(Joypad{
          .joystick = joystick,
          .axis_x = axis_x,
          .axis_y = axis_y,
      });
      break;
    }

    default:
      break;
    }
  }
  state.current.game = (state.keyboard | state.current.pad);
  return state.current;
}

const InputSnapshot &InputSystem::Current() const { return impl_->current; }

std::optional<InputPadButton> InputSystem::PadSingle() const {
  for (const auto &pad : impl_->pads) {
    if (pad.buttons_held > 1) {
      return 0;
    }
    if (pad.buttons_held == 1) {
      return pad.button_pressed_last;
    }
  }
  return std::nullopt;
}
