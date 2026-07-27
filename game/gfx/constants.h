///
/// Shared graphics types and constants — used by both game/ and GIAN07/
///
#pragma once

#include <cstdint>

#include "coords.h"

// The game's native resolution.
constexpr WINDOW_SIZE GRP_RES = {.w = 640, .h = 480};
inline constexpr WINDOW_LTRB GRP_RES_RECT = {{.x = 0, .y = 0}, GRP_RES};

constexpr auto GAME_TITLE = "秋霜玉";
constexpr auto VERSION_TAG = "v1.0";

// Maximum number of triangles rendered in a single draw call.
constexpr auto GRP_TRIANGLES_MAX = 66;

// Yup, the game is supposed to be running at 62.5 FPS.
constexpr auto FRAME_TIME_TARGET = 16;

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

// Surface layer IDs for the blitting pipeline
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
  BOMBER = 6,  // Bomb graphics

  // Splash screen
  SPROJECT = 1, // Western Project display

  // Endings
  ENDING_CREDITS = 1,
  ENDING_PIC = 2,

  // Rendered text. Since this one is procedurally generated and therefore
  // doesn't have a palette, it must come last to ensure that DirectDraw
  // initializes it with the implicit palette loaded for an earlier surface.
  TEXT = 8,

  COUNT = 9,
};

// Font IDs for text rendering
enum class FONT_ID : uint8_t {
  // IDs referenced by original game data
  SMALL = 0,  // Font (small characters)
  NORMAL = 1, // Font (normal characters)
  LARGE = 2,  // Font (large characters)

  // Newly added in this fork
  TINY = 3,

  COUNT = 4,
};
