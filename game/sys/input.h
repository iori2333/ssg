///
/// Cross-platform input declarations
///
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <utility>

// Current pressed/released state for all virtual KEY_* keys.
using INPUT_BITS = uint16_t;

// 0 = unmapped.
using INPUT_PAD_BUTTON = uint8_t;

// Keyboard constants
// Braced initializers cause a compile error if the constants don't fit within
// the INPUT_BITS type.
constexpr INPUT_BITS KEY_UP = {0x0001};
constexpr INPUT_BITS KEY_DOWN = {0x0002};
constexpr INPUT_BITS KEY_LEFT = {0x0004};
constexpr INPUT_BITS KEY_RIGHT = {0x0008};
constexpr INPUT_BITS KEY_TAMA = {0x0010};
constexpr INPUT_BITS KEY_BOMB = {0x0020};
constexpr INPUT_BITS KEY_SHIFT = {0x0040};
constexpr INPUT_BITS KEY_RETURN = {0x0080};
constexpr INPUT_BITS KEY_ESC = {0x0100};
constexpr INPUT_BITS KEY_STAGE1 = {0x0200};
constexpr INPUT_BITS KEY_STAGE2 = {0x0400};
constexpr INPUT_BITS KEY_STAGE3 = {0x0800};
constexpr INPUT_BITS KEY_STAGE4 = {0x1000};
constexpr INPUT_BITS KEY_STAGE5 = {0x2000};
constexpr INPUT_BITS KEY_STAGE6 = {0x4000};
// Stored in Demo Replay input streams as the visible playback start marker.
constexpr INPUT_BITS KEY_DEMO_START = {0x8000};

constexpr INPUT_BITS KEY_ULEFT = (KEY_UP | KEY_LEFT);
constexpr INPUT_BITS KEY_URIGHT = (KEY_UP | KEY_RIGHT);
constexpr INPUT_BITS KEY_DLEFT = (KEY_DOWN | KEY_LEFT);
constexpr INPUT_BITS KEY_DRIGHT = (KEY_DOWN | KEY_RIGHT);

// Returns whether this key represents an "OK" action.
bool Input_IsOK(INPUT_BITS key);

// Returns whether this key represents a "Cancel" action.
bool Input_IsCancel(INPUT_BITS key);

// Returns the delta that this key would apply to a numeric option value.
int_fast8_t Input_OptionKeyDelta(INPUT_BITS key);

// Additional virtual keys for inputs that were read using GetAsyncKeyState()
// in the original game. Treated separately to not complicate any existing
// comparisons of the regular game input with 0.
using INPUT_SYSTEM_BITS = uint16_t;

constexpr INPUT_SYSTEM_BITS SYSKEY_SNAPSHOT = {0x0001};
constexpr INPUT_SYSTEM_BITS SYSKEY_SKIP = {0x0002};
constexpr INPUT_SYSTEM_BITS SYSKEY_BGM_FADE = {0x0004};
constexpr INPUT_SYSTEM_BITS SYSKEY_BGM_DEVICE = {0x0008};
constexpr INPUT_SYSTEM_BITS SYSKEY_DEMO_RECORD = {0x0010};

using INPUT_PAD_BINDING = std::pair<INPUT_PAD_BUTTON, INPUT_BITS>;

struct InputSnapshot {
  INPUT_BITS game = 0;
  INPUT_BITS pad = 0;
  INPUT_SYSTEM_BITS system = 0;
};

class InputSystem {
public:
  InputSystem();
  ~InputSystem();
  InputSystem(const InputSystem &) = delete;
  InputSystem(InputSystem &&) = delete;
  InputSystem &operator=(const InputSystem &) = delete;
  InputSystem &operator=(InputSystem &&) = delete;

  void SetPadBindings(std::span<const INPUT_PAD_BINDING> bindings);
  [[nodiscard]] InputSnapshot Poll();
  [[nodiscard]] const InputSnapshot &Current() const;

  // >= 1: one button; 0: multiple buttons; nullopt: no button.
  [[nodiscard]] std::optional<INPUT_PAD_BUTTON> PadSingle() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
