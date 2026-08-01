///
/// Shared graphics types and constants — used by both game/ and GIAN07/
///
#pragma once

#include <cstdint>
#include <string_view>

#include "coords.h"

// The game's native resolution.
constexpr WindowSize kGameResolution = {.w = 640, .h = 480};
inline constexpr WindowLtrb kGameResolutionRect = {{.x = 0, .y = 0},
                                                   kGameResolution};

constexpr std::string_view kGameTitle = "秋霜玉";
constexpr std::string_view kVersionTag = GAME_VERSION;

// Maximum number of triangles rendered in a single draw call.
constexpr auto kMaxTriangles = 66;

// Yup, the game is supposed to be running at 62.5 FPS.
constexpr auto kFrameTimeTarget = 16;

// At least on Windows, SDL 3's default graphics API (Direct3D 11) also appears
// to be the most performant choice:
//
// 	https://rec98.nmlgc.net/blog/2025-04-09#sdl3-2025-04-09
//
// Hence, Windows builds also get pixel-perfect line rendering compared to
// pbg's original build by default:
//
// 	https://rec98.nmlgc.net/blog/2024-10-22#lines-2024-10-22
constexpr const char *kSdlDefaultApi = nullptr;

// Surface layer IDs for the blitting pipeline
enum class SurfaceId : uint8_t {
  System = 0, // System

  // Title Screen
  Title = 2, // Title screen

  // Music Room
  Music = 2, // Music room

  // Name Registration
  NameRegistration = 2, // Name registration

  // In-game
  MapChip = 1, // Background
  Enemy = 2,   // Enemies (trash & bosses)
  Face = 3,    // Face graphics
  Bomber = 6,  // Bomb graphics

  // Splash screen
  Project = 1, // Western Project display

  // Endings
  EndingCredits = 1,
  EndingPicture = 2,

  // Rendered text. Since this one is procedurally generated and therefore
  // doesn't have a palette, it must come last to ensure that DirectDraw
  // initializes it with the implicit palette loaded for an earlier surface.
  Text = 8,

  Count = 9,
};

// Font IDs for text rendering
enum class FontId : uint8_t {
  // IDs referenced by original game data
  Small = 0,  // Font (small characters)
  Normal = 1, // Font (normal characters)
  Large = 2,  // Font (large characters)

  // Newly added in this fork
  Tiny = 3,

  Count = 4,
};
