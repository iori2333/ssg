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
// 0 = unmapped.
// Keyboard constants
// Braced initializers cause a compile error if the constants don't fit within
// the InputBits type.
using InputBits = uint16_t;
using InputPadButton = uint8_t;

constexpr InputBits KeyUp = {0x0001};
constexpr InputBits KeyDown = {0x0002};
constexpr InputBits KeyLeft = {0x0004};
constexpr InputBits KeyRight = {0x0008};
constexpr InputBits KeyTama = {0x0010};
constexpr InputBits KeyBomb = {0x0020};
constexpr InputBits KeyShift = {0x0040};
constexpr InputBits KeyReturn = {0x0080};
constexpr InputBits KeyEscape = {0x0100};
constexpr InputBits KeyStage1 = {0x0200};
constexpr InputBits KeyStage2 = {0x0400};
constexpr InputBits KeyStage3 = {0x0800};
constexpr InputBits KeyStage4 = {0x1000};
constexpr InputBits KeyStage5 = {0x2000};
constexpr InputBits KeyStage6 = {0x4000};
// Stored in Demo Replay input streams as the visible playback start marker.
constexpr InputBits KeyDemoStart = {0x8000};

constexpr InputBits KeyUpLeft = (KeyUp | KeyLeft);
constexpr InputBits KeyUpRight = (KeyUp | KeyRight);
constexpr InputBits KeyDownLeft = (KeyDown | KeyLeft);
constexpr InputBits KeyDownRight = (KeyDown | KeyRight);

// Returns whether this key represents an "OK" action.
bool InputIsOk(InputBits key);

// Returns whether this key represents a "Cancel" action.
bool InputIsCancel(InputBits key);

// Returns the delta that this key would apply to a numeric option value.
int_fast8_t InputOptionKeyDelta(InputBits key);

// Additional virtual keys for inputs that were read using GetAsyncKeyState()
// in the original game. Treated separately to not complicate any existing
// comparisons of the regular game input with 0.
using InputSystemBits = uint16_t;

constexpr InputSystemBits SystemKeySnapshot = {0x0001};
constexpr InputSystemBits SystemKeySkip = {0x0002};
constexpr InputSystemBits SystemKeyBgmFade = {0x0004};
constexpr InputSystemBits SystemKeyBgmDevice = {0x0008};
constexpr InputSystemBits SystemKeyDemoRecord = {0x0010};

using InputPadBinding = std::pair<InputPadButton, InputBits>;

struct InputSnapshot {
  InputBits game = 0;
  InputBits pad = 0;
  InputSystemBits system = 0;
};

class InputSystem {
public:
  InputSystem();
  ~InputSystem();
  InputSystem(const InputSystem &) = delete;
  InputSystem(InputSystem &&) = delete;
  InputSystem &operator=(const InputSystem &) = delete;
  InputSystem &operator=(InputSystem &&) = delete;

  void SetPadBindings(std::span<const InputPadBinding> bindings);
  [[nodiscard]] InputSnapshot Poll();
  [[nodiscard]] const InputSnapshot &Current() const;

  // >= 1: one button; 0: multiple buttons; nullopt: no button.
  [[nodiscard]] std::optional<InputPadButton> PadSingle() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
