///
/// EffectManager lifecycle, screen transitions, and boss warning overlay.
///

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ranges>

#include "effect_manager.h"
#include "effect_types.h"

#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

namespace {

struct Point3D {
  int x = 0;
  int y = 0;
  int z = 0;
};

void RotatePoint(Point3D &point, uint8_t angle_x, uint8_t angle_y,
                 uint8_t angle_z) {
  auto rotated = math::RoundedRotateVector(math::AngleFromLegacy(angle_x),
                                           point.y, point.z);
  point.y = rotated.x;
  point.z = rotated.y;

  rotated = math::RoundedRotateVector(-math::AngleFromLegacy(angle_y), point.x,
                                      point.z);
  point.x = rotated.x;
  point.z = rotated.y;

  rotated = math::RoundedRotateVector(math::AngleFromLegacy(angle_z), point.x,
                                      point.y);
  point.x = rotated.x;
  point.y = rotated.y;
}

} // namespace

void EffectManager::Reset() {
  ResetStrings();
  ResetCircles();
  ResetFragments();
  ResetBombExplosions();
  ResetScreenTransition();
  ResetBossWarning();
}

void EffectManager::Update() {
  UpdateFragments();
  UpdateStrings();
  UpdateCircles();
  UpdateBombExplosions();
  UpdateBossWarning();
  UpdateScreenTransition();
}

void EffectManager::UpdateGameOver() {
  UpdateFragments();
  UpdateStrings();
}

void EffectManager::DrawForeground() {
  DrawBossWarning();
  DrawStrings();
}

void EffectManager::ResetScreenTransition() {
  screen_ = {};
  GraphicsBackendSetClip(playfield::kClip);
}

void EffectManager::StartScreenTransition(ScreenTransition transition) {
  screen_ = {.transition = transition, .age = 0, .active = true};
}

void EffectManager::UpdateScreenTransition() {
  if (!screen_.active) {
    return;
  }

  switch (screen_.transition) {
  case ScreenTransition::CircleFadeIn:
    screen_.age += 10;
    if (screen_.age > 600) {
      screen_.active = false;
    }
    break;
  case ScreenTransition::CircleFadeOut:
    screen_.age = std::min(screen_.age + 10, 600);
    break;
  case ScreenTransition::WhiteIn:
    screen_.age = std::min(screen_.age + 10, 150);
    break;
  case ScreenTransition::WhiteOut:
    screen_.age += 10;
    if (screen_.age >= 160) {
      screen_.active = false;
    }
    break;
  }
}

void EffectManager::DrawScreenTransition() const {
  if (!screen_.active) {
    return;
  }

  switch (screen_.transition) {
  case ScreenTransition::CircleFadeIn:
    DrawCircleFade(playfield::kCenterX, playfield::kCenterY, screen_.age);
    return;
  case ScreenTransition::CircleFadeOut:
    DrawCircleFade(playfield::kCenterX, playfield::kCenterY,
                   400 - screen_.age);
    return;
  case ScreenTransition::WhiteIn:
  case ScreenTransition::WhiteOut:
    break;
  }

  const int frame = screen_.transition == ScreenTransition::WhiteIn
                        ? 15 - screen_.age / 10
                        : screen_.age / 10;
  const PixelLtwh source = {frame * 16, 144, 16, 16};
  for (int x = playfield::kLeft; x <= playfield::kRight; x += 16) {
    for (int y = playfield::kTop; y <= playfield::kBottom; y += 16) {
      GraphicsSurfaceBlit({x, y}, SurfaceId::System, source);
    }
  }
}

void EffectManager::DrawCircleFade(int x, int y, int radius) {
  radius = std::max(radius, 0);
  for (int tile_x = 0; tile_x < kGameResolution.w; tile_x += 16) {
    for (int tile_y = 0; tile_y < kGameResolution.h; tile_y += 16) {
      const int dx = tile_x - x;
      const int dy = tile_y - y;
      const int distance =
          static_cast<int>(std::lround(std::sqrt(dx * dx + dy * dy)));
      if (distance < radius && radius - distance < 8 * 16) {
        const PixelLtwh source = {((radius - distance) >> 3) << 4, 128, 16, 16};
        GraphicsSurfaceBlit({tile_x, tile_y}, SurfaceId::System, source);
      } else if (distance >= radius) {
        GraphicsSurfaceBlit({tile_x, tile_y}, SurfaceId::System,
                            PixelLtwh{0, 128, 16, 16});
      }
    }
  }

  if (radius == 0) {
    GraphicsBackendSetClip({playfield::kCenterX, playfield::kCenterY,
                            playfield::kCenterX, playfield::kCenterY});
    return;
  }
  GraphicsBackendSetClip(
      {std::clamp(x - radius, playfield::kLeft, playfield::kRight + 1),
       std::max(y - radius, playfield::kTop),
       std::clamp(x + radius + 1, playfield::kLeft, playfield::kRight + 1),
       std::min(y + radius + 1, playfield::kBottom + 1)});
}

void EffectManager::ResetBossWarning() {
  warning_active_ = false;
  warning_age_ = 0;
  warning_pulse_ = 0;
  InitializeWarningText();
}

void EffectManager::StartBossWarning() {
  warning_active_ = true;
  warning_age_ = 0;
  InitializeWarningText();
}

void EffectManager::UpdateBossWarning() {
  if (!warning_active_) {
    return;
  }

  if (warning_age_ < 192) {
    UpdateWarningText(warning_age_);
  } else {
    RotateWarningText(-1);
  }

  if (warning_age_++ == 266) {
    warning_active_ = false;
  }
}

void EffectManager::DrawBossWarning() {
  if (!warning_active_) {
    return;
  }

  if (warning_age_ < 236) {
    DrawWarningText();
  }
  if (warning_age_ <= 216) {
    return;
  }

  int radius = (warning_age_ - 216) * 3;
  constexpr std::array colors = {Rgb216{1, 1, 5}, Rgb216{2, 2, 5},
                                 Rgb216{3, 3, 5}, Rgb216{4, 4, 5},
                                 Rgb216{5, 5, 5}};
  constexpr std::array radius_steps = {4, 4, 6, 6, 8};
  for (std::size_t index = 0; index < colors.size(); ++index) {
    radius -= radius_steps[index];
    geometry::SetColor(colors[index]);
    geometry::DrawCircle({320, 100}, radius);
  }
}

void EffectManager::InitializeWarningText() {
  warning_w_ = {PixelPoint{.x = 0, .y = 15},  PixelPoint{.x = 15, .y = 66},
                PixelPoint{.x = 32, .y = 47}, PixelPoint{.x = 48, .y = 66},
                PixelPoint{.x = 63, .y = 14}, PixelPoint{.x = 52, .y = 11},
                PixelPoint{.x = 42, .y = 38}, PixelPoint{.x = 32, .y = 26},
                PixelPoint{.x = 21, .y = 38}, PixelPoint{.x = 11, .y = 10},
                PixelPoint{.x = 0, .y = 15}};
  warning_a_outer_ = {
      PixelPoint{.x = 96, .y = 12},  PixelPoint{.x = 66, .y = 61},
      PixelPoint{.x = 75, .y = 67},  PixelPoint{.x = 83, .y = 56},
      PixelPoint{.x = 107, .y = 56}, PixelPoint{.x = 115, .y = 67},
      PixelPoint{.x = 125, .y = 61}, PixelPoint{.x = 96, .y = 12}};
  warning_a_inner_ = {
      PixelPoint{.x = 96, .y = 34}, PixelPoint{.x = 90, .y = 44},
      PixelPoint{.x = 101, .y = 44}, PixelPoint{.x = 96, .y = 34}};
  warning_r_ = {PixelPoint{.x = 132, .y = 14}, PixelPoint{.x = 132, .y = 64},
                PixelPoint{.x = 145, .y = 64}, PixelPoint{.x = 145, .y = 27},
                PixelPoint{.x = 164, .y = 27}, PixelPoint{.x = 150, .y = 42},
                PixelPoint{.x = 171, .y = 66}, PixelPoint{.x = 173, .y = 66},
                PixelPoint{.x = 181, .y = 57}, PixelPoint{.x = 167, .y = 43},
                PixelPoint{.x = 180, .y = 29}, PixelPoint{.x = 180, .y = 27},
                PixelPoint{.x = 170, .y = 14}, PixelPoint{.x = 132, .y = 14}};
  const std::array n_points = {
      PixelPoint{.x = 189, .y = 12}, PixelPoint{.x = 189, .y = 64},
      PixelPoint{.x = 201, .y = 64}, PixelPoint{.x = 201, .y = 40},
      PixelPoint{.x = 239, .y = 66}, PixelPoint{.x = 239, .y = 14},
      PixelPoint{.x = 227, .y = 14}, PixelPoint{.x = 227, .y = 38},
      PixelPoint{.x = 189, .y = 12}};
  std::ranges::transform(n_points, warning_n_left_.begin(),
                         [](PixelPoint point) { return WorldPoint{point}; });
  warning_n_right_ = warning_n_left_;
  warning_i_ = {PixelPoint{.x = 248, .y = 14}, PixelPoint{.x = 248, .y = 64},
                PixelPoint{.x = 262, .y = 64}, PixelPoint{.x = 262, .y = 14},
                PixelPoint{.x = 248, .y = 14}};
  warning_g_ = {PixelPoint{.x = 354, .y = 11}, PixelPoint{.x = 328, .y = 22},
                PixelPoint{.x = 328, .y = 57}, PixelPoint{.x = 354, .y = 68},
                PixelPoint{.x = 380, .y = 59}, PixelPoint{.x = 380, .y = 34},
                PixelPoint{.x = 355, .y = 34}, PixelPoint{.x = 354, .y = 45},
                PixelPoint{.x = 367, .y = 46}, PixelPoint{.x = 367, .y = 51},
                PixelPoint{.x = 355, .y = 55}, PixelPoint{.x = 342, .y = 50},
                PixelPoint{.x = 342, .y = 29}, PixelPoint{.x = 354, .y = 24},
                PixelPoint{.x = 372, .y = 30}, PixelPoint{.x = 377, .y = 19},
                PixelPoint{.x = 354, .y = 11}};

  warning_lines_ = {
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_w_},
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_a_outer_},
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_a_inner_},
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_r_},
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_n_left_},
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_i_},
      WarningLine{.center = {.x = 192 - (296 - 215), .y = 39},
                  .points = warning_n_right_},
      WarningLine{.center = {.x = 192, .y = 39}, .points = warning_g_},
  };
  for (auto &line : warning_lines_) {
    const WorldPoint center{line.center};
    for (auto &point : line.points) {
      point -= center;
    }
  }
}

void EffectManager::UpdateWarningText(int age) {
  for (auto &line : warning_lines_) {
    line.angle_x = age < 64 ? static_cast<uint8_t>((64 - age) * 2) : 0;
    line.angle_y = age < 64 ? static_cast<uint8_t>(64 - age) : 0;
    line.angle_z = age < 64 ? static_cast<uint8_t>((64 - age) * 4) : 0;
  }
}

void EffectManager::RotateWarningText(int amount) {
  if (amount == 0) {
    return;
  }
  for (auto &line : warning_lines_) {
    line.angle_x = static_cast<uint8_t>(line.angle_x + amount * 2);
    line.angle_y = static_cast<uint8_t>(line.angle_y + amount);
    line.angle_z = static_cast<uint8_t>(line.angle_z + amount * 4);
  }
}

void EffectManager::DrawWarningText() {
  warning_pulse_ += 8;
  if (warning_lines_[0].angle_x == 0) {
    const auto pulse = math::RoundedPolarVector(
        static_cast<float>(warning_pulse_) * math::kLegacyAngleStep, 48.0F);
    geometry::SetAlphaNorm(static_cast<uint8_t>(128 + pulse.y));
    geometry::SetColor({5, 0, 0});
    geometry::DrawBoxA(129, 46, 512, 66);
    geometry::DrawBoxA(129, 136, 512, 156);
    GraphicsSurfaceBlit({129, 61}, SurfaceId::System,
                        PixelLtrb{0, 168, 384, 248});
    return;
  }

  int start_rotation = -8;
  int rotation_step = 2;
  if (warning_lines_[0].angle_x < 10) {
    start_rotation = 0;
    rotation_step = 0;
  } else if (warning_lines_[0].angle_x < 20) {
    start_rotation = -4;
    rotation_step = 1;
  }

  const auto draw_lines = [this] {
    for (const auto &line : warning_lines_) {
      const auto rotate = [&line](const WorldPoint &point) {
        Point3D transformed{.x = point.x, .y = point.y, .z = 0};
        RotatePoint(transformed, line.angle_x, line.angle_y, line.angle_z);
        return WorldPoint::FromWorld(transformed.x, transformed.y);
      };
      auto previous = rotate(line.points.front());
      for (const auto &point : line.points | std::views::drop(1)) {
        const auto current = rotate(point);
        const auto p1 = PixelPoint{.x = 320, .y = 100} + previous.ToPixel();
        const auto p2 = PixelPoint{.x = 320, .y = 100} + current.ToPixel();
        geometry::DrawLine(p1.x, p1.y, p2.x, p2.y);
        previous = current;
      }
    }
  };

  const std::array colors = {Rgb216{1, 1, 5}, Rgb216{2, 2, 5}, Rgb216{3, 3, 5},
                             Rgb216{4, 4, 5}, Rgb216{5, 5, 5}};
  RotateWarningText(start_rotation);
  for (std::size_t index = 0; index < colors.size(); ++index) {
    geometry::SetColor(colors[index]);
    draw_lines();
    if (index + 1 < colors.size()) {
      RotateWarningText(rotation_step);
    }
  }
}
