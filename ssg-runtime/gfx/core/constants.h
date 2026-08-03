///
/// Shared graphics types and constants used by the runtime and game.
///
#pragma once

#include <cstdint>
#include <string_view>

#include "coords.h"
#include "rect.h"

// The game's native resolution.

constexpr PixelPoint kGameResolution = {.x = 640, .y = 480};
inline constexpr Rect kGameResolutionRect =
    Rect::FromPositionAndSize({}, kGameResolution);

constexpr std::string_view kGameTitle = "秋霜玉";
constexpr std::string_view kVersionTag = GAME_VERSION;

// Maximum number of triangles rendered in a single draw call.
constexpr auto kMaxTriangles = 66;

// Yup, the game is supposed to be running at 62.5 FPS.
constexpr auto kFrameTimeTarget = 16;

// 0 = BMP, 10 = max-effort WebP.
constexpr int kScreenshotEffortMax = 10;

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

  Count = 8,
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
