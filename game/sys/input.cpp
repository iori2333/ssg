///
/// Cross-platform input
///

#include <cassert>
#include <limits>
#include <optional>
#include <ranges>
#include <tuple>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include "input.h"

#include "util/enum_flags.h"
#include "util/guard.h"

// MSVC's static analyzer suggests to make the functions below `constexpr`,
// which won't work because they are used in other translation units and this
// is not a header.
#pragma warning(suppress : 26497) // f.4
bool Input_IsOK(INPUT_BITS key) {
  return ((key == KEY_RETURN) || (key == KEY_TAMA));
}

#pragma warning(suppress : 26497) // f.4
bool Input_IsCancel(INPUT_BITS key) {
  return ((key == KEY_BOMB) || (key == KEY_ESC));
}

int_fast8_t Input_OptionKeyDelta(INPUT_BITS key) {
  return ((Input_IsOK(key) || (key == KEY_RIGHT)) ? 1
          : (key == KEY_LEFT)                     ? -1
                                                  : 0);
}

///
/// Input via SDL events
///

// Do scancodes and key modes still fit into 16 bits each?
static_assert(SDL_SCANCODE_COUNT <= std::numeric_limits<uint16_t>::max());
static_assert(SDL_KMOD_SCROLL <= std::numeric_limits<uint16_t>::max());

enum class KEY_MOD : uint8_t {
  HAS_BITFLAG_OPERATORS,
  NONE = 0x00,
  LALT = 0x01,
};

// In-game scancode, received from SDL.
struct KEY_SCANCODE {
  uint16_t scancode;
  KEY_MOD mod;
};

// Static key binding.
struct KEY_BIND {
  uint16_t scancode;

  // Modifiers that *must* match.
  KEY_MOD mod_must = KEY_MOD::NONE;

  // Modifiers that *must not* match. Should contain all modifiers of other
  // bindings that share the same [scancode] to allow the matching to work
  // independently of the order within the binding array.
  KEY_MOD mod_filter = KEY_MOD::NONE;

  constexpr KEY_BIND(SDL_Scancode scancode, KEY_MOD mod_must = KEY_MOD::NONE,
                     KEY_MOD mod_filter = KEY_MOD::NONE)
      : scancode(scancode), mod_must(mod_must), mod_filter(mod_filter) {}

  bool Matches(const KEY_SCANCODE &sc) const {
    return ((scancode == sc.scancode) && !(sc.mod & mod_filter) &&
            ((mod_must & sc.mod) == mod_must));
  }
};

static constexpr std::pair<KEY_BIND, INPUT_BITS> KeyBindings[] = {
    {SDL_SCANCODE_UP, KEY_UP},
    {SDL_SCANCODE_DOWN, KEY_DOWN},
    {SDL_SCANCODE_LEFT, KEY_LEFT},
    {SDL_SCANCODE_RIGHT, KEY_RIGHT},
    {SDL_SCANCODE_KP_1, KEY_DLEFT},
    {SDL_SCANCODE_KP_2, KEY_DOWN},
    {SDL_SCANCODE_KP_3, KEY_DRIGHT},
    {SDL_SCANCODE_KP_4, KEY_LEFT},
    {SDL_SCANCODE_KP_6, KEY_RIGHT},
    {SDL_SCANCODE_KP_7, KEY_ULEFT},
    {SDL_SCANCODE_KP_8, KEY_UP},
    {SDL_SCANCODE_KP_9, KEY_URIGHT},
    {SDL_SCANCODE_Z, KEY_TAMA},
    {SDL_SCANCODE_X, KEY_BOMB},
    {SDL_SCANCODE_LSHIFT, KEY_SHIFT},
    {SDL_SCANCODE_ESCAPE, KEY_ESC},
    {SDL_SCANCODE_1, KEY_STAGE1},
    {SDL_SCANCODE_2, KEY_STAGE2},
    {SDL_SCANCODE_3, KEY_STAGE3},
    {SDL_SCANCODE_4, KEY_STAGE4},
    {SDL_SCANCODE_5, KEY_STAGE5},
    {SDL_SCANCODE_6, KEY_STAGE6},

    {{SDL_SCANCODE_RETURN, KEY_MOD::NONE, KEY_MOD::LALT}, KEY_RETURN},
};
static constexpr std::pair<KEY_BIND, INPUT_SYSTEM_BITS> SystemKeyBindings[] = {
    {SDL_SCANCODE_P, SYSKEY_SNAPSHOT},   {SDL_SCANCODE_LCTRL, SYSKEY_SKIP},
    {SDL_SCANCODE_RCTRL, SYSKEY_SKIP},   {SDL_SCANCODE_F, SYSKEY_BGM_FADE},
    {SDL_SCANCODE_D, SYSKEY_BGM_DEVICE}, {SDL_SCANCODE_R, SYSKEY_DEMO_RECORD},
};

template <typename Bits>
void Key_Flip(Bits &key_data, const auto &key_or_jbutton, Bits bits) {
  if (key_or_jbutton.down) {
    key_data |= bits;
  } else {
    key_data &= ~bits;
  }
}

struct JOYPAD {
  SDL_Joystick *joystick;
  uint8_t axis_x;
  uint8_t axis_y;
  INPUT_PAD_BUTTON button_pressed_last = 0;
  uint8_t buttons_held = 0;
};

struct InputSystem::Impl {
  InputSnapshot current;
  INPUT_BITS keyboard = 0;
  std::vector<JOYPAD> pads;
  std::vector<INPUT_PAD_BINDING> pad_bindings;
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
std::tuple<decltype(JOYPAD::axis_x), decltype(JOYPAD::axis_y)>
Pad_GetAxisIDs(int device_index) {
  decltype(JOYPAD::axis_x) axis_x = 0;
  decltype(JOYPAD::axis_y) axis_y = 1;

  if (!SDL_IsGamepad(device_index)) {
    return {axis_x, axis_y};
  }
  auto *gamepad = SDL_OpenGamepad(device_index);
  if (!gamepad) {
    return {axis_x, axis_y};
  }
  auto gamepad_guard = make_guard(gamepad, SDL_CloseGamepad);

  int binding_count = 0;
  auto **bindings = SDL_GetGamepadBindings(gamepad, &binding_count);
  if (!bindings) {
    return {axis_x, axis_y};
  }
  auto bindings_guard = make_guard(bindings, SDL_free);

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
  return {axis_x, axis_y};
}

std::vector<JOYPAD>::iterator Pad_Find(std::vector<JOYPAD> &pads,
                                       SDL_JoystickID id) {
  auto *joystick = SDL_GetJoystickFromID(id);
  const auto it = std::ranges::find_if(
      pads, [joystick](const auto &pad) { return (pad.joystick == joystick); });
  assert(it != pads.end());
  return it;
}

InputSystem::InputSystem() : impl_(std::make_unique<Impl>()) {}

InputSystem::~InputSystem() {
  for (auto &pad : impl_->pads) {
    SDL_CloseJoystick(pad.joystick);
  }
}

void InputSystem::SetPadBindings(std::span<const INPUT_PAD_BINDING> bindings) {
  impl_->pad_bindings.assign(bindings.begin(), bindings.end());
}

InputSnapshot InputSystem::Poll() {
  auto &state = *impl_;
  SDL_Event event;
  while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_KEY_DOWN,
                        SDL_EVENT_JOYSTICK_REMOVED) == 1) {
    switch (event.type) {
    case SDL_EVENT_JOYSTICK_AXIS_MOTION: {
      auto &pad = *Pad_Find(state.pads, event.jaxis.which);

      // The original WinMM backend did this without even taking the
      // range reported by joyGetDevCaps() into account. However, that
      // function returns a range of [0, 65535] with all joypads I've
      // tried, which matches SDL's fixed [-32768, 32767] range, so we
      // can do it here as well.
      const auto v = (event.jaxis.value >> 11);
      if (event.jaxis.axis == pad.axis_x) {
        state.current.pad &= ~(KEY_LEFT | KEY_RIGHT);
        state.current.pad |=
            ((v <= -4) ? KEY_LEFT : ((v >= 4) ? KEY_RIGHT : 0));
      } else if (event.jaxis.axis == pad.axis_y) {
        state.current.pad &= ~(KEY_UP | KEY_DOWN);
        state.current.pad |= ((v <= -4) ? KEY_UP : ((v >= 4) ? KEY_DOWN : 0));
      }
      break;
    }

    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP: {
      // SDL's numbering starts at 0.
      const INPUT_PAD_BUTTON id = (event.jbutton.button + 1);
      for (const auto &binding : state.pad_bindings) {
        if (id == binding.first) {
          Key_Flip(state.current.pad, event.jbutton, binding.second);
        }
      }

      auto &pad = *Pad_Find(state.pads, event.jbutton.which);
      using HELD = std::numeric_limits<decltype(pad.button_pressed_last)>;
      if (event.jbutton.down) {
        if (pad.buttons_held < HELD::max()) {
          pad.buttons_held++;
        }
        pad.button_pressed_last = id;
      } else {
        if (pad.buttons_held > HELD::min()) {
          pad.buttons_held--;
        }
      }
      break;
    }

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      const auto mod = SDL_GetModState();

      const KEY_SCANCODE scancode = {
          .scancode = static_cast<uint16_t>(event.key.scancode),
          .mod = ((mod & SDL_KMOD_LALT) ? KEY_MOD::LALT : KEY_MOD::NONE),
      };
      for (const auto &binding : KeyBindings) {
        if (binding.first.Matches(scancode)) {
          Key_Flip(state.keyboard, event.key, binding.second);
        }
      }
      for (const auto &binding : SystemKeyBindings) {
        if (binding.first.Matches(scancode)) {
          Key_Flip(state.current.system, event.key, binding.second);
        }
      }
      break;
    }

    case SDL_EVENT_JOYSTICK_REMOVED: {
      const auto pad = Pad_Find(state.pads, event.jdevice.which);
      SDL_CloseJoystick(pad->joystick);
      if (pad != (state.pads.end() - 1)) {
        *pad = state.pads.back();
      }
      state.pads.pop_back();
      break;
    }

    case SDL_EVENT_JOYSTICK_ADDED: {
      auto *joystick = SDL_OpenJoystick(event.jdevice.which);
      if (!joystick) {
        break;
      }
      const auto [axis_x, axis_y] = Pad_GetAxisIDs(event.jdevice.which);
      state.pads.emplace_back(JOYPAD{
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

std::optional<INPUT_PAD_BUTTON> InputSystem::PadSingle() const {
  for (const auto &pad : impl_->pads) {
    if (pad.buttons_held > 1) {
      return 0;
    } else if (pad.buttons_held == 1) {
      return pad.button_pressed_last;
    }
  }
  return std::nullopt;
}
