///
/// EffectManager lifecycle, screen transitions, and boss warning overlay.
///

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>

#include "effect_manager.h"

#include "gameplay/playfield.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "util/cast.h"
#include "util/ut_math.h"

namespace {

struct Point3D {
  int x = 0;
  int y = 0;
  int z = 0;
};

void RotatePoint(Point3D &point, uint8_t angle_x, uint8_t angle_y,
                 uint8_t angle_z) {
  auto old_y = point.y;
  auto old_z = point.z;
  point.y = cosl(angle_x, old_y) - sinl(angle_x, old_z);
  point.z = sinl(angle_x, old_y) + cosl(angle_x, old_z);

  auto old_x = point.x;
  old_z = point.z;
  point.x = cosl(angle_y, old_x) + sinl(angle_y, old_z);
  point.z = -sinl(angle_y, old_x) + cosl(angle_y, old_z);

  old_x = point.x;
  old_y = point.y;
  point.x = cosl(angle_z, old_x) - sinl(angle_z, old_y);
  point.y = sinl(angle_z, old_x) + cosl(angle_z, old_y);
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
  GrpBackend_SetClip(playfield::kClip);
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
  const PIXEL_LTWH source = {frame * 16, 144, 16, 16};
  for (int x = playfield::kLeft; x <= playfield::kRight; x += 16) {
    for (int y = playfield::kTop; y <= playfield::kBottom; y += 16) {
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, source);
    }
  }
}

void EffectManager::DrawCircleFade(int x, int y, int radius) {
  radius = std::max(radius, 0);
  for (int tile_x = 0; tile_x < GRP_RES.w; tile_x += 16) {
    for (int tile_y = 0; tile_y < GRP_RES.h; tile_y += 16) {
      const int dx = tile_x - x;
      const int dy = tile_y - y;
      const int distance = isqrt(dx * dx + dy * dy);
      if (distance < radius && radius - distance < 8 * 16) {
        const PIXEL_LTWH source = {((radius - distance) >> 3) << 4, 128, 16,
                                   16};
        GrpSurface_Blit({tile_x, tile_y}, SURFACE_ID::SYSTEM, source);
      } else if (distance >= radius) {
        GrpSurface_Blit({tile_x, tile_y}, SURFACE_ID::SYSTEM,
                        PIXEL_LTWH{0, 128, 16, 16});
      }
    }
  }

  if (radius == 0) {
    GrpBackend_SetClip({playfield::kCenterX, playfield::kCenterY,
                        playfield::kCenterX, playfield::kCenterY});
    return;
  }
  GrpBackend_SetClip(
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
    UpdateWarningText(Cast::down<uint8_t>(warning_age_));
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

  GrpGeom->Lock();
  int radius = (warning_age_ - 216) * 3;
  constexpr std::array colors = {RGB216{1, 1, 5}, RGB216{2, 2, 5},
                                 RGB216{3, 3, 5}, RGB216{4, 4, 5},
                                 RGB216{5, 5, 5}};
  constexpr std::array radius_steps = {4, 4, 6, 6, 8};
  for (std::size_t index = 0; index < colors.size(); ++index) {
    radius -= radius_steps[index];
    GrpGeom->SetColor(colors[index]);
    GeomCircle({320, 100}, radius);
  }
  GrpGeom->Unlock();
}

void EffectManager::InitializeWarningText() {
  warning_w_ = {PIXEL_POINT{0, 15},  PIXEL_POINT{15, 66}, PIXEL_POINT{32, 47},
                PIXEL_POINT{48, 66}, PIXEL_POINT{63, 14}, PIXEL_POINT{52, 11},
                PIXEL_POINT{42, 38}, PIXEL_POINT{32, 26}, PIXEL_POINT{21, 38},
                PIXEL_POINT{11, 10}, PIXEL_POINT{0, 15}};
  warning_a_outer_ = {PIXEL_POINT{96, 12},  PIXEL_POINT{66, 61},
                      PIXEL_POINT{75, 67},  PIXEL_POINT{83, 56},
                      PIXEL_POINT{107, 56}, PIXEL_POINT{115, 67},
                      PIXEL_POINT{125, 61}, PIXEL_POINT{96, 12}};
  warning_a_inner_ = {PIXEL_POINT{96, 34}, PIXEL_POINT{90, 44},
                      PIXEL_POINT{101, 44}, PIXEL_POINT{96, 34}};
  warning_r_ = {
      PIXEL_POINT{132, 14}, PIXEL_POINT{132, 64}, PIXEL_POINT{145, 64},
      PIXEL_POINT{145, 27}, PIXEL_POINT{164, 27}, PIXEL_POINT{150, 42},
      PIXEL_POINT{171, 66}, PIXEL_POINT{173, 66}, PIXEL_POINT{181, 57},
      PIXEL_POINT{167, 43}, PIXEL_POINT{180, 29}, PIXEL_POINT{180, 27},
      PIXEL_POINT{170, 14}, PIXEL_POINT{132, 14}};
  const std::array n_points = {
      PIXEL_POINT{189, 12}, PIXEL_POINT{189, 64}, PIXEL_POINT{201, 64},
      PIXEL_POINT{201, 40}, PIXEL_POINT{239, 66}, PIXEL_POINT{239, 14},
      PIXEL_POINT{227, 14}, PIXEL_POINT{227, 38}, PIXEL_POINT{189, 12}};
  std::ranges::transform(n_points, warning_n_left_.begin(),
                         [](PIXEL_POINT point) { return WORLD_POINT{point}; });
  warning_n_right_ = warning_n_left_;
  warning_i_ = {PIXEL_POINT{248, 14}, PIXEL_POINT{248, 64},
                PIXEL_POINT{262, 64}, PIXEL_POINT{262, 14},
                PIXEL_POINT{248, 14}};
  warning_g_ = {
      PIXEL_POINT{354, 11}, PIXEL_POINT{328, 22}, PIXEL_POINT{328, 57},
      PIXEL_POINT{354, 68}, PIXEL_POINT{380, 59}, PIXEL_POINT{380, 34},
      PIXEL_POINT{355, 34}, PIXEL_POINT{354, 45}, PIXEL_POINT{367, 46},
      PIXEL_POINT{367, 51}, PIXEL_POINT{355, 55}, PIXEL_POINT{342, 50},
      PIXEL_POINT{342, 29}, PIXEL_POINT{354, 24}, PIXEL_POINT{372, 30},
      PIXEL_POINT{377, 19}, PIXEL_POINT{354, 11}};

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
    const WORLD_POINT center{line.center};
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
    GrpGeom->Lock();
    GrpGeom->SetAlphaNorm(
        Cast::down_sign<uint8_t>(128 + sinl(warning_pulse_, 48)));
    GrpGeom->SetColor({5, 0, 0});
    GrpGeom->DrawBoxA(129, 46, 512, 66);
    GrpGeom->DrawBoxA(129, 136, 512, 156);
    GrpGeom->Unlock();
    GrpSurface_Blit({129, 61}, SURFACE_ID::SYSTEM,
                    PIXEL_LTRB{0, 168, 384, 248});
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
      const auto rotate = [&line](const WORLD_POINT &point) {
        Point3D transformed{point.x, point.y, 0};
        RotatePoint(transformed, line.angle_x, line.angle_y, line.angle_z);
        return WORLD_POINT::FromWorld(transformed.x, transformed.y);
      };
      auto previous = rotate(line.points.front());
      for (const auto &point : line.points | std::views::drop(1)) {
        const auto current = rotate(point);
        const auto p1 = PIXEL_POINT{320, 100} + previous.ToPixel();
        const auto p2 = PIXEL_POINT{320, 100} + current.ToPixel();
        GrpGeom->DrawLine(p1.x, p1.y, p2.x, p2.y);
        previous = current;
      }
    }
  };

  GrpGeom->Lock();
  const std::array colors = {RGB216{1, 1, 5}, RGB216{2, 2, 5}, RGB216{3, 3, 5},
                             RGB216{4, 4, 5}, RGB216{5, 5, 5}};
  RotateWarningText(start_rotation);
  for (std::size_t index = 0; index < colors.size(); ++index) {
    GrpGeom->SetColor(colors[index]);
    draw_lines();
    if (index + 1 < colors.size()) {
      RotateWarningText(rotation_step);
    }
  }
  GrpGeom->Unlock();
}
