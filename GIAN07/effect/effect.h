///
/// Effect - Effect-related definitions
///

#pragma once

#include <array>
#include <cstdint>

// [Change history]
// 2000/04/28 : Created circle effect
// 2000/04/15 : Added circular fade functions
// 2000/02/23 : Development started (->Ver0.20)

// [Constants]
inline constexpr auto SEFFECT_MAX = 1000;
inline constexpr auto LOCKON_MAX = 2;      // Max lock-on count
inline constexpr auto CIRCLE_EFC_MAX = 10; // Max circle effects

inline constexpr auto CEFC_NONE = 0x00;    // CircleEffect not in use
inline constexpr auto CEFC_STAR = 0x01;    // Star-like effect
inline constexpr auto CEFC_CIRCLE1 = 0x02; // Converging circle effect
inline constexpr auto CEFC_CIRCLE2 = 0x03; // Diverging circle effect

inline constexpr auto SEFC_NONE = 0x00;   // Not used
inline constexpr auto SEFC_STR1 = 0x01;   // String effect 1
inline constexpr auto SEFC_STR1_2 = 0x02; // String pause
inline constexpr auto SEFC_STR1_3 = 0x03; // String explosion

inline constexpr auto SEFC_MTITLE1 = 0x04; // Music title effect (start)
inline constexpr auto SEFC_MTITLE2 = 0x05; // Music title effect (stop)
inline constexpr auto SEFC_MTITLE3 = 0x06; // Music title effect (retreat)

inline constexpr auto SEFC_GAMEOVER = 0x07;  // Warning display etc.
inline constexpr auto SEFC_GAMEOVER2 = 0x08; // Warning display etc.

inline constexpr auto SEFC_STR2 = 0x10; // Score item effect?

inline constexpr auto LOCKON_NONE = 0x00; // Not locked
inline constexpr auto LOCKON_01 = 0x01;   // Lock-on start
inline constexpr auto LOCKON_02 = 0x02;   // Lock-on stop
inline constexpr auto LOCKON_03 = 0x03;   // Lock-on release?

inline constexpr auto SCNEFC_NONE = 0x00;     // No effect
inline constexpr auto SCNEFC_CFADEIN = 0x01;  // Circular fade-in
inline constexpr auto SCNEFC_CFADEOUT = 0x02; // Circular fade-out
inline constexpr auto SCNEFC_WHITEIN = 0x03;  // White-in
inline constexpr auto SCNEFC_WHITEOUT = 0x04; // White-out

// [Structs]
struct CircleEffectData {
  int x, y;       // Center coordinates
  int r, rmax;    // Radius / max radius
  uint32_t count; // Counter
  uint8_t type;   // Circle effect type
                  // uint8_t Level; // Circle effect level
  double d;      // Circle effect angle (radians)
};
// (CIRCLE_EFC_DATA alias removed — use CircleEffectData directly)

struct StringEffectData {
  int x, y;
  int vx, vy;

  uint32_t time;
  uint32_t point;

  uint8_t cmd;
  char c;
};
// (SEFFECT_DATA alias removed — use StringEffectData directly)

struct LockOnInfo {
  int *x, *y;        // Pointer to lock-on coordinates
  int width, height; // Width and height
  int vx, vy;        // Velocity components
  uint32_t count;    // Counter
  uint8_t state;     // State
};
// (LOCKON_INFO alias removed — use LockOnInfo directly)

struct ScreenEffectState {
  uint8_t cmd;    // Active effect
  uint32_t count; // Effect counter
};
// (SCREENEFC_INFO alias removed — use ScreenEffectState directly)

// [Functions]
// Backward-compatible inline wrappers moved to end of effect_manager.h
void GrpDrawSpect(int x, int y); // Spectrum analyzer draw (MUSIC.cpp)
void GrpDrawNote();              // Display pressed keys (MUSIC.cpp)

// [Variables]
// All effect variables -> access directly via Effects.xxx (see
// effect_manager.h)
