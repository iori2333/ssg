///
/// Constants - Game-specific compile-time constants and types shared with
/// platform layers
///

#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

#include "gfx/coords.h"

// The game's native resolution.
constexpr WINDOW_SIZE GRP_RES = {.w = 640, .h = 480};
constexpr WINDOW_LTRB GRP_RES_RECT = {{.x = 0, .y = 0}, GRP_RES};
constexpr auto GAME_ORG = "rec98";
constexpr auto GAME_APP = "sh01";
constexpr auto GAME_TITLE = "秋霜玉";
constexpr auto VERSION_TAG = "v1.0";

// Maximum number of triangles rendered in a single draw call.
constexpr auto GRP_TRIANGLES_MAX = 66;

// Yup, the game is supposed to be running at 62.5 FPS.
constexpr auto FRAME_TIME_TARGET = 16;

// ID types

constexpr auto FACE_MAX = 3; // Maximum simultaneous face loads
constexpr auto ENDING_PIC_MAX = 6;

enum class SURFACE_ID : uint8_t {
  SYSTEM = 0, // System

  // Title Screen
  TITLE = 2, // Title screen

  // Music Room
  MUSIC = 2, // Music room

  // Name Registration
  NAMEREG = 2, // Name registration

  // In-game
  MAPCHIP = 1, // Background
  ENEMY = 2,   // Enemies (trash & bosses)
  FACE = 3,    // Face graphics
  FACE_last = (FACE + FACE_MAX - 1),
  BOMBER = 6, // Bomb graphics

  // Splash screen
  SPROJECT = 1, // Western Project display

  // Endings
  ENDING_CREDITS = 1,
  ENDING_PIC = 2,
  ENDING_PIC_last = (ENDING_PIC + ENDING_PIC_MAX - 1),

  // Rendered text. Since this one is procedurally generated and therefore
  // doesn't have a palette, it must come last to ensure that DirectDraw
  // initializes it with the implicit palette loaded for an earlier surface.
  TEXT = 8,

  COUNT = 9,
};

// Addition is only defined for the types of surfaces we have multiple of.
static constexpr SURFACE_ID operator+(SURFACE_ID lhs, uint8_t rhs) {
  assert(!((lhs == SURFACE_ID::FACE) && (rhs >= FACE_MAX)));
  assert(!((lhs == SURFACE_ID::ENDING_PIC) && (rhs >= ENDING_PIC_MAX)));
  return SURFACE_ID{static_cast<uint8_t>(std::to_underlying(lhs) + rhs)};
}

enum class FONT_ID : uint8_t {
  // IDs referenced by original game data
  SMALL = 0,  // Font (small characters)
  NORMAL = 1, // Font (normal characters)
  LARGE = 2,  // Font (large characters)

  // Newly added in this fork
  TINY = 3,

  COUNT = 4,
};

// Mapping world coordinates to a position in the stereo field
// These constants map the [x] parameter from any source unit to a position in
// the stereo field. The resulting unit is the attenuation volume of either the
// right (negative) or left (positive) channel, expressed in decibels.
// The algorithm from the original game:
//
// • The [x] values are world coordinates (Q26.6, 64 units per pixel)
// • Subtract the center of the screen (in world coordinates) from [x]
// • Divide the result by (a scalar) 16
// • Directly pass that result to DirectSound, which interprets it as a panning
//   value with a unit of 1/100 dB
//
// By transforming the calculation to pixel space and full decibels, we end up
// with ((16 / 64) × 100) = 25 pixels per shifted decibel.

// Default X-coordinate center
constexpr int SND_X_MID = PixelToWorld(320);

constexpr int SND_X_PER_DECIBEL = PixelToWorld(25);

// At least on Windows, SDL 3's default graphics API (Direct3D 11) also appears
// to be the most performant choice:
//
// 	https://rec98.nmlgc.net/blog/2025-04-09#sdl3-2025-04-09
//
// Hence, Windows builds also get pixel-perfect line rendering compared to
// pbg's original build by default:
//
// 	https://rec98.nmlgc.net/blog/2024-10-22#lines-2024-10-22
constexpr const char *GRP_SDL_DEFAULT_API = nullptr;
