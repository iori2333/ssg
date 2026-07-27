///
/// HomingLoadout - tracking shots and focused straight columns.
///

#include "homing_loadout.h"

#include "enemy/enemy_manager.h"
#include "gfx/coords.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

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

void HomingLoadout::FireMain(Player &player, uint8_t tier, bool focused) {
  if (focused) {
    FireMainFocused(player, tier);
  } else {
    FireMainNormal(player, tier);
  }
}

void HomingLoadout::FireSub(Player &player, uint8_t tier, bool focused) {
  if (focused) {
    FireSubFocused(player, tier);
  } else {
    FireSubNormal(player, tier);
  }
}

void HomingLoadout::FireMainNormal(Player &player_, uint8_t tier) {
  switch (tier) {
  case 0: {
    shot_phase_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(shot_phase_, 4));
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), static_cast<uint8_t>(-64 + dd), 0, 1,
        13.5_px,     0,           PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    break;
  }
  case 1: {
    PlayerShotSpawnInfo si{
        player_.X() - 6_px,        player_.Y(), 192, 0, 1, 13.5_px, 0,
        PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    si.x += 12_px;
    player_.SpawnShot(si);
    break;
  }
  case 2:
  case 3: {
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), 192, 7,
        3,           13.5_px,     0,   PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), 192, 7,
        5,           13.5_px,     0,   PlayerShotKind::HomingMain};
    player_.SpawnShot(si);
    break;
  }
  }
}

void HomingLoadout::FireSubNormal(Player &player_, uint8_t tier) {
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
                               uint16_t remaining) {
  if (remaining % 30 == 1) {
    PlayerShotSpawnInfo si{
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

void HomingLoadout::FireMainFocused(Player &player_, uint8_t tier) {
  const int count = (tier <= 0) ? 1 : (tier <= 2) ? 2 : (tier <= 4) ? 3 : 4;
  const int spread = (count - 1) * 12_px;

  PlayerShotSpawnInfo si{
      player_.X() - spread / 2,       player_.Y(), 192, 0, 1, 13.5_px, 0,
      PlayerShotKind::HomingFocusMain};
  for (int i = 0; i < count; i++) {
    player_.SpawnShot(si);
    si.x += 12_px;
  }
}

void HomingLoadout::FireSubFocused(Player &player_, uint8_t tier) {
  if (tier == 0) {
    return;
  }

  PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                         player_.OpY(),
                         192,
                         0,
                         1,
                         13.5_px,
                         0,
                         PlayerShotKind::HomingFocusSub};
  player_.SpawnShot(si);

  si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
  player_.SpawnShot(si);
}
