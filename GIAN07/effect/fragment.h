///
/// Fragment - Fragment processing functions
///

#pragma once

#include <array>
#include <cstdint>

// Fragment constants
inline constexpr auto FRAGMENT_MAX = 1000;  // Maximum number of fragments
inline constexpr auto FRG_EVADE = 0x00;     // Graze (under development)
inline constexpr auto FRG_SMOKE = 0x01;     // Smoke 1
inline constexpr auto FRG_FATCIRCLE = 0x02; // Red circle...
inline constexpr auto FRG_STAR1 = 0x03;     // Star 1
inline constexpr auto FRG_STAR2 = 0x04;     // Star 2
inline constexpr auto FRG_HIT = 0x05;       // Shot hit
inline constexpr auto FRG_STAR3 = 0x06;
inline constexpr auto FRG_HEART = 0x07; // Heart shape

inline constexpr auto FRG_ESCAPE = 0x10;   // Escape from specified coordinates
inline constexpr auto FRG_APPROACH = 0x20; // Approach specified coordinates

// Fragment data structure
struct FragmentData {
  int x, y;      // Current coordinates
  int vx, vy;    // Velocity components (x64)
  uint8_t count; // Frame counter (0 means not in use)
  uint8_t cmd;   // Fragment type
};
// (FRAGMENT_DATA alias removed — use FragmentData directly)

// Fragment variables
// Fragment[], FragmentPtr → Effects.fragments, Effects.fragment_ptr
// Access directly

// Fragment functions
// Backward compatibility inline wrapper moved to end of effect_manager.h
