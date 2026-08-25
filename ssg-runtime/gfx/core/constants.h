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

// Surface layer IDs for the blitting pipeline. Each value must be unique:
// RendererState::textures is an EnumArray indexed by SurfaceId, so duplicate
// values would alias the same texture slot.
enum class SurfaceId : uint8_t {
  System = 0,          // System surface (fonts, small sprites)
  MapChip = 1,         // Background tiles
  Title = 2,           // Title screen
  Music = 3,           // Music room
  NameRegistration = 4, // Name registration
  Enemy = 5,           // Enemies (trash & bosses)
  Bomber = 6,          // Bomb graphics
  Project = 7,         // Western Project display (splash)
  EndingCredits = 8,   // Ending credits
  // Multi-surface ranges (see data::graphics_assets):
  //   EndingPicture = 9 .. 14 (6 pictures)
  //   Face = 15 .. 17        (3 face graphics)
  EndingPicture = 9,
  Face = 15,
  Count = 18,
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
