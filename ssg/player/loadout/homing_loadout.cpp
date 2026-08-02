///
/// HomingLoadout - tracking shots and focused straight columns.
///

#include <cmath>
#include <cstdint>

#include "homing_loadout.h"
#include "player_loadout.h"

#include "enemy/enemy_manager.h"
#include "gfx/coords.h"
#include "player/player.h"
#include "player/player_shot.h"
#include "util/math_utils.h"

namespace {
constexpr PlayerTraits kHomingTraits{
    .type = PlayerType::Homing,
    .move_speed = 18_px,
    .focus_move_speed = 6_px,
    .hit_radius = 1.5_px,
    .bomb_duration = 60 * 3,
    .option_sprite = 1,
    .option_offset = 26,
    .focus_option_offset = 26,
};
} // namespace

HomingLoadout::HomingLoadout() : PlayerLoadout(kHomingTraits) {}

void HomingLoadout::FireMain(Player &player, int tier, bool focused) {
  if (focused) {
    FireMainFocused(player, tier);
  } else {
    FireMainNormal(player, tier);
  }
}

void HomingLoadout::FireSub(Player &player, int tier, bool focused) {
  if (focused) {
    FireSubFocused(player, tier);
  } else {
    FireSubNormal(player, tier);
  }
}

void HomingLoadout::FireMainNormal(Player &player_, int tier) {
  switch (tier) {
  case 0: {
    shot_phase_ += 32;
    const auto dd = static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        4.0F));
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = static_cast<uint8_t>(-64 + dd),
                                 .direction_step = 0,
                                 .count = 1,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    break;
  }
  case 1: {
    PlayerShotSpawnInfo si{.x = player_.X() - 6_px,
                           .y = player_.Y(),
                           .direction = 192,
                           .direction_step = 0,
                           .count = 1,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    si.x += 12_px;
    player_.SpawnShot(si);
    break;
  }
  case 2:
  case 3: {
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = 192,
                                 .direction_step = 7,
                                 .count = 3,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = 192,
                                 .direction_step = 7,
                                 .count = 5,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    break;
  }
  }
}

void HomingLoadout::FireSubNormal(Player &player_, int tier) {
  if (tier == 0) {
    return;
  }

  PlayerShotSpawnInfo si{};
  si.motion = PlayerShotMotion::Homing;
  si.turn_rate = 5;
  si.speed = 7_px;
  si.acceleration = 4;
  si.kind = PlayerShotKind::HomingSub;

  si.x = player_.OpX() + PixelToWorld(OptionOffset(false));
  si.y = player_.OpY();
  if (tier < 8) {
    si.direction = 59;
    si.direction_step = 0;
    si.count = 1;
  } else {
    si.direction = 42;
    si.direction_step = 30;
    si.count = 2;
  }
  player_.SpawnShot(si);

  si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
  if (tier < 8) {
    si.direction = 69;
    si.direction_step = 0;
  } else {
    si.direction = 86;
    si.direction_step = 30;
  }
  player_.SpawnShot(si);
}

void HomingLoadout::UpdateBomb(Player &player_, EnemyManager & /*enemies*/,
                               EffectManager & /*effects*/,
                               int remaining) {
  if (remaining % 30 == 1) {
    PlayerShotSpawnInfo const si{
        .x = player_.X(),
        .y = player_.Y(),
        .direction = 64,
        .direction_step = 16,
        .count = 8,
        .speed = 7_px,
        .acceleration = 4,
        .kind = PlayerShotKind::HomingBomb,
        .motion = PlayerShotMotion::Homing,
        .turn_rate = 5,
    };
    player_.SpawnShot(si);
  }
}

void HomingLoadout::FireMainFocused(Player &player_, int tier) {
  int count = 1;
  if (tier > 0 && tier <= 2) {
    count = 2;
  } else if (tier > 2 && tier <= 4) {
    count = 3;
  } else if (tier > 4) {
    count = 4;
  }
  const int spread = (count - 1) * 12_px;

  PlayerShotSpawnInfo si{.x = player_.X() - spread / 2,
                         .y = player_.Y(),
                         .direction = 192,
                         .direction_step = 0,
                         .count = 1,
                         .speed = 13.5_px,
                         .acceleration = 0,
                         .kind = PlayerShotKind::HomingFocusMain};
  for (int i = 0; i < count; i++) {
    player_.SpawnShot(si);
    si.x += 12_px;
  }
}

void HomingLoadout::FireSubFocused(Player &player_, int tier) {
  if (tier == 0) {
    return;
  }

  PlayerShotSpawnInfo si{.x = player_.OpX() + PixelToWorld(OptionOffset(false)),
                         .y = player_.OpY(),
                         .direction = 192,
                         .direction_step = 0,
                         .count = 1,
                         .speed = 13.5_px,
                         .acceleration = 0,
                         .kind = PlayerShotKind::HomingFocusSub};
  player_.SpawnShot(si);

  si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
  player_.SpawnShot(si);
}
