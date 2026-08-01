///
/// EffectManager lifecycle, screen transitions, and boss warning overlay.
///

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <ranges>

#include "effect_manager.h"

#include "gameplay/playfield.h"
#include "gfx/geometry.h"
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
    screen_.age = std::min<uint32_t>(screen_.age + 10, 600);
    break;
  case ScreenTransition::WhiteIn:
    screen_.age = std::min<uint32_t>(screen_.age + 10, 150);
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
    DrawCircleFade(playfield::kCenterX, playfield::kCenterY,
                   static_cast<int>(screen_.age));
    return;
  case ScreenTransition::CircleFadeOut:
    DrawCircleFade(playfield::kCenterX, playfield::kCenterY,
                   400 - static_cast<int>(screen_.age));
    return;
  case ScreenTransition::WhiteIn:
  case ScreenTransition::WhiteOut:
    break;
  }

  const int frame = screen_.transition == ScreenTransition::WhiteIn
                        ? 15 - static_cast<int>(screen_.age / 10)
                        : static_cast<int>(screen_.age / 10);
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
    UpdateWarningText(static_cast<uint8_t>(warning_age_));
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
    Geometry().SetColor(colors[index]);
    GeomCircle({320, 100}, radius);
  }
}

void EffectManager::InitializeWarningText() {
  warning_w_ = {PixelPoint{0, 15},  PixelPoint{15, 66}, PixelPoint{32, 47},
                PixelPoint{48, 66}, PixelPoint{63, 14}, PixelPoint{52, 11},
                PixelPoint{42, 38}, PixelPoint{32, 26}, PixelPoint{21, 38},
                PixelPoint{11, 10}, PixelPoint{0, 15}};
  warning_a_outer_ = {PixelPoint{96, 12},  PixelPoint{66, 61},
                      PixelPoint{75, 67},  PixelPoint{83, 56},
                      PixelPoint{107, 56}, PixelPoint{115, 67},
                      PixelPoint{125, 61}, PixelPoint{96, 12}};
  warning_a_inner_ = {PixelPoint{96, 34}, PixelPoint{90, 44},
                      PixelPoint{101, 44}, PixelPoint{96, 34}};
  warning_r_ = {PixelPoint{132, 14}, PixelPoint{132, 64}, PixelPoint{145, 64},
                PixelPoint{145, 27}, PixelPoint{164, 27}, PixelPoint{150, 42},
                PixelPoint{171, 66}, PixelPoint{173, 66}, PixelPoint{181, 57},
                PixelPoint{167, 43}, PixelPoint{180, 29}, PixelPoint{180, 27},
                PixelPoint{170, 14}, PixelPoint{132, 14}};
  const std::array n_points = {
      PixelPoint{189, 12}, PixelPoint{189, 64}, PixelPoint{201, 64},
      PixelPoint{201, 40}, PixelPoint{239, 66}, PixelPoint{239, 14},
      PixelPoint{227, 14}, PixelPoint{227, 38}, PixelPoint{189, 12}};
  std::ranges::transform(n_points, warning_n_left_.begin(),
                         [](PixelPoint point) { return WorldPoint{point}; });
  warning_n_right_ = warning_n_left_;
  warning_i_ = {PixelPoint{248, 14}, PixelPoint{248, 64}, PixelPoint{262, 64},
                PixelPoint{262, 14}, PixelPoint{248, 14}};
  warning_g_ = {PixelPoint{354, 11}, PixelPoint{328, 22}, PixelPoint{328, 57},
                PixelPoint{354, 68}, PixelPoint{380, 59}, PixelPoint{380, 34},
                PixelPoint{355, 34}, PixelPoint{354, 45}, PixelPoint{367, 46},
                PixelPoint{367, 51}, PixelPoint{355, 55}, PixelPoint{342, 50},
                PixelPoint{342, 29}, PixelPoint{354, 24}, PixelPoint{372, 30},
                PixelPoint{377, 19}, PixelPoint{354, 11}};

  warning_lines_ = {
      WarningLine{{192, 39}, warning_w_},
      WarningLine{{192, 39}, warning_a_outer_},
      WarningLine{{192, 39}, warning_a_inner_},
      WarningLine{{192, 39}, warning_r_},
      WarningLine{{192, 39}, warning_n_left_},
      WarningLine{{192, 39}, warning_i_},
      WarningLine{{192 - (296 - 215), 39}, warning_n_right_},
      WarningLine{{192, 39}, warning_g_},
  };
  for (auto &line : warning_lines_) {
    const WorldPoint center{line.center};
    for (auto &point : line.points) {
      point -= center;
    }
  }
}

void EffectManager::UpdateWarningText(uint8_t age) {
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
        static_cast<float>(warning_pulse_) * math::kLegacyAngleStep, 48.0f);
    Geometry().SetAlphaNorm(static_cast<uint8_t>(128 + pulse.y));
    Geometry().SetColor({5, 0, 0});
    Geometry().DrawBoxA(129, 46, 512, 66);
    Geometry().DrawBoxA(129, 136, 512, 156);
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
        Point3D transformed{point.x, point.y, 0};
        RotatePoint(transformed, line.angle_x, line.angle_y, line.angle_z);
        return WorldPoint::FromWorld(transformed.x, transformed.y);
      };
      auto previous = rotate(line.points.front());
      for (const auto &point : line.points | std::views::drop(1)) {
        const auto current = rotate(point);
        const auto p1 = PixelPoint{320, 100} + previous.ToPixel();
        const auto p2 = PixelPoint{320, 100} + current.ToPixel();
        Geometry().DrawLine(p1.x, p1.y, p2.x, p2.y);
        previous = current;
      }
    }
  };

  const std::array colors = {Rgb216{1, 1, 5}, Rgb216{2, 2, 5}, Rgb216{3, 3, 5},
                             Rgb216{4, 4, 5}, Rgb216{5, 5, 5}};
  RotateWarningText(start_rotation);
  for (std::size_t index = 0; index < colors.size(); ++index) {
    Geometry().SetColor(colors[index]);
    draw_lines();
    if (index + 1 < colors.size()) {
      RotateWarningText(rotation_step);
    }
  }
}
