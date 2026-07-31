///
/// LaserLoadout - straight shots, continuous option beams, and laser bomb.
///

#include <algorithm>

#include "laser_loadout.h"

#include "enemy/enemy_manager.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "player/player_attack.h"
#include "util/math_utils.h"

namespace {
constexpr PlayerTraits kLaserTraits{
    .type = PlayerType::Laser,
    .move_speed = 21_px,
    .focus_move_speed = 7_px,
    .hit_radius = 1.5_px,
    .bomb_duration = 60 * 2,
    .option_sprite = 2,
    .option_offset = 26,
    .focus_option_offset = 13,
};
} // namespace

LaserLoadout::LaserLoadout() : PlayerLoadout(kLaserTraits) {}

void LaserLoadout::FireMain(Player &player, uint8_t tier, bool focused) {
  switch (tier) {
  case 0: {
    player.SpawnShot({player.X(), player.Y(), 192, 0, 1, 13.5_px, 0,
                      PlayerShotKind::LaserSub});
    break;
  }
  case 1:
  case 2: {
    PlayerShotSpawnInfo shot{
        player.X() - 6_px,       player.Y(), 192, 0, 1, 13.5_px, 0,
        PlayerShotKind::LaserSub};
    player.SpawnShot(shot);
    shot.x += 12_px;
    player.SpawnShot(shot);
    StartBeam(player, 64 + 50);
    break;
  }
  case 3:
  case 4:
    player.SpawnShot({player.X(), player.Y(), 192,
                      static_cast<uint8_t>(focused ? 2 : 6), 3, 13.5_px, 0,
                      PlayerShotKind::LaserSub});
    StartBeam(player, 64 + 100);
    break;
  case 5:
  case 6:
  case 7: {
    PlayerShotSpawnInfo shot{player.X() - 6_px,
                             player.Y(),
                             static_cast<uint8_t>(focused ? 190 : 187),
                             static_cast<uint8_t>(focused ? 4 : 10),
                             2,
                             13.5_px,
                             0,
                             PlayerShotKind::LaserSub};
    player.SpawnShot(shot);
    shot.x += 12_px;
    shot.direction = static_cast<uint8_t>(focused ? 194 : 197);
    player.SpawnShot(shot);
    StartBeam(player, 64 + 150);
    break;
  }
  default:
    player.SpawnShot({player.X(), player.Y(), 192,
                      static_cast<uint8_t>(focused ? 2 : 6),
                      static_cast<uint8_t>(focused ? 4 : 5), 13.5_px, 0,
                      PlayerShotKind::LaserSub});
    StartBeam(player, 64 + 200);
    break;
  }
}

void LaserLoadout::StartBeam(const Player &player, uint16_t time) {
  if (player.IsBombActive() || player.IsMovementDisabled()) {
    Reset();
    return;
  }
  if (beam_time_ == 0) {
    beam_time_ = time;
  }
}

void LaserLoadout::Tick(Player & /*player*/) {
  if (beam_time_ == 0) {
    return;
  }

  beam_time_--;
  if (beam_time_ < 64) {
    beam_group_ = 0;
  } else if (beam_time_ < 64 + 50) {
    beam_group_ = 1;
  } else if (beam_time_ < 64 + 100) {
    beam_group_ = 2;
  } else if (beam_time_ < 64 + 150) {
    beam_group_ = 3;
  } else {
    beam_group_ = 4;
  }
}

void LaserLoadout::ApplyContinuousAttack(const Player &player,
                                         EnemyManager &enemies,
                                         bool focused) const {
  if (beam_group_ == 0) {
    return;
  }

  const int offset = OptionOffset(focused);
  const int damage = (beam_group_ / 3) + 1;
  enemies.ApplyPlayerAttack(PlayerAttack::VerticalBeam(
      WORLD_POINT::FromWorld(player.OpX() + PixelToWorld(offset), player.OpY()),
      damage));
  enemies.ApplyPlayerAttack(PlayerAttack::VerticalBeam(
      WORLD_POINT::FromWorld(player.OpX() - PixelToWorld(offset), player.OpY()),
      damage));
}

uint8_t LaserLoadout::BombAngle(uint16_t remaining) const {
  return static_cast<uint8_t>(((BombDuration() - remaining) * 3) / 2);
}

uint8_t LaserLoadout::LeftOrRightAngle(uint8_t angle, int index) {
  return static_cast<uint8_t>(
      (angle < 58)
          ? ((angle * 3) + ((index * (64 - angle)) / 2))
          : ((58 * 3) +
             ((index * (64 - std::min(62, static_cast<int>(angle)))) / 2)));
}

uint8_t LaserLoadout::RightAngle(uint8_t angle, int index) {
  return static_cast<uint8_t>(64 + 48 - LeftOrRightAngle(angle, index));
}

uint8_t LaserLoadout::LeftAngle(uint8_t angle, int index) {
  return static_cast<uint8_t>(64 - 48 + LeftOrRightAngle(angle, index));
}

void LaserLoadout::UpdateBomb(Player &player, EnemyManager &enemies,
                              EffectManager & /*effects*/, uint16_t remaining) {
  const auto angle = BombAngle(remaining);

  const int right_x = player.OpX() + PixelToWorld(OptionOffset(false));
  for (int i = -3; i <= 3; i++) {
    enemies.ApplyPlayerAttack(PlayerAttack::DirectedBeam(
        WORLD_POINT::FromWorld(right_x, player.OpY()), RightAngle(angle, i)));
  }

  const int left_x = player.OpX() - PixelToWorld(OptionOffset(false));
  for (int i = -3; i <= 3; i++) {
    enemies.ApplyPlayerAttack(PlayerAttack::DirectedBeam(
        WORLD_POINT::FromWorld(left_x, player.OpY()), LeftAngle(angle, i)));
  }
}

void LaserLoadout::DrawBombForeground(const Player &player,
                                      uint16_t remaining) const {
  if (remaining == 0) {
    return;
  }

  constexpr RGBA color = RGB216{0, 0, 5}.ToRGB().WithAlpha(0xFF);
  const auto angle = BombAngle(remaining);
  VERTEX_XY points[4];
  const auto set_points = [&](int origin_x, int origin_y, int length_x,
                              int length_y, int width_x, int width_y) {
    points[0].x = origin_x + width_x;
    points[0].y = origin_y + width_y;
    points[3].x = origin_x - width_x;
    points[3].y = origin_y - width_y;
    points[2].x = origin_x + length_x - width_x;
    points[2].y = origin_y + length_y - width_y;
    points[1].x = origin_x + length_x + width_x;
    points[1].y = origin_y + length_y + width_y;
  };

  const auto draw = [&](int width) {
    Geometry().SetAlphaOne();
    for (const int side : {1, -1}) {
      for (int i = -3; i <= 3; i++) {
        const auto direction =
            side > 0 ? RightAngle(angle, i) : LeftAngle(angle, i);
        const auto length =
            math::RoundedPolarVector(math::AngleFromLegacy(direction), 850.0f);
        const auto beam_width = math::RoundedPolarVector(
            math::AngleFromLegacy(static_cast<uint8_t>(direction + 64)), width);
        const int origin_x = (player.OpX() >> 6) + side * OptionOffset(false);
        const int origin_y = player.OpY() >> 6;
        set_points(origin_x, origin_y, length.x, length.y, beam_width.x,
                   beam_width.y);
        GeomGrdRectA(Geometry(), points, color);
      }
    }
  };

  if (angle < 58) {
    draw(3);
  } else if (angle < 150) {
    draw(12 - ((angle - 64) / 8));
  }
}

void LaserLoadout::DrawContinuousAttack(const Player &player,
                                        bool focused) const {
  if (beam_group_ == 0) {
    return;
  }

  const int offset = OptionOffset(focused);
  auto source = PIXEL_LTWH{384 + ((beam_group_ - 1) << 4), 240, 8, 16};
  for (const int side : {-1, 1}) {
    GrpSurface_Blit(
        {(player.OpX() >> 6) - 4 + side * offset, (player.OpY() >> 6) - 20},
        SURFACE_ID::SYSTEM, source);
  }

  source = PIXEL_LTWH{384 + 8 + ((beam_group_ - 1) << 4), 240, 8, 16};
  for (const int side : {-1, 1}) {
    for (int y = (player.OpY() >> 6) - 36; y > -16; y -= 16) {
      GrpSurface_Blit({(player.OpX() >> 6) - 4 + side * offset, y},
                      SURFACE_ID::SYSTEM, source);
    }
  }
}

void LaserLoadout::ClearContinuousAttack() {
  beam_time_ = 0;
  beam_group_ = 0;
}

void LaserLoadout::Reset() { ClearContinuousAttack(); }
