///
/// WideLoadout - spread shot and focused straight columns.
///

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "wide_loadout.h"

#include "effect/effect_manager.h"
#include "effect/effect_types.h"
#include "enemy/enemy_manager.h"
#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/graphics_backend.h"
#include "player/loadout/player_loadout.h"
#include "player/player.h"
#include "player/player_attack.h"
#include "player/player_shot.h"
#include "util/math_utils.h"

namespace {
constexpr PlayerTraits kWideTraits{
    .type = PlayerType::Wide,
    .move_speed = 15_px,
    .focus_move_speed = 5_px,
    .hit_radius = 1.5_px,
    .bomb_duration = 60 * 4,
    .option_sprite = 0,
    .option_offset = 26,
    .focus_option_offset = 26,
};
} // namespace

WideLoadout::WideLoadout() : PlayerLoadout(kWideTraits) {}

void WideLoadout::FireMain(Player &player, uint8_t tier, bool focused) {
  if (focused) {
    FireMainFocused(player, tier);
  } else {
    FireMainNormal(player, tier);
  }
}

void WideLoadout::FireSub(Player &player, uint8_t tier, bool focused) {
  if (focused) {
    FireSubFocused(player, tier);
  } else {
    FireSubNormal(player, tier);
  }
}

void WideLoadout::FireMainNormal(Player &player_, uint8_t tier) {
  switch (tier) {
  case 0: {
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = 192,
                                 .direction_step = 0,
                                 .count = 1,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  case 1: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0F)));
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = static_cast<uint8_t>(-64 + dd),
                                 .direction_step = 0,
                                 .count = 1,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  case 2: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0F)));
    PlayerShotSpawnInfo si{.x = player_.X() - 6_px,
                           .y = player_.Y(),
                           .direction = static_cast<uint8_t>(-64 + dd),
                           .direction_step = 0,
                           .count = 1,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    si.x += 12_px;
    player_.SpawnShot(si);
    break;
  }
  case 3:
  case 4:
  case 5: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0F)));
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = static_cast<uint8_t>(-64 + dd),
                                 .direction_step = 4,
                                 .count = 3,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  default: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0F)));
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = static_cast<uint8_t>(-64 + dd),
                                 .direction_step = 3,
                                 .count = 5,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  }
}

void WideLoadout::FireSubNormal(Player &player_, uint8_t tier) {
  switch (tier) {
  case 0:
    break;
  case 1:
  case 2:
  case 3: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 197,
                           .direction_step = 0,
                           .count = 1,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 187;
    player_.SpawnShot(si);
    break;
  }
  case 4:
  case 5: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 200,
                           .direction_step = 7,
                           .count = 2,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 200 - 16;
    player_.SpawnShot(si);
    break;
  }
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 202,
                           .direction_step = 8,
                           .count = 3,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 202 - 20;
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 204,
                           .direction_step = 8,
                           .count = 4,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 204 - 24;
    player_.SpawnShot(si);
    break;
  }
  }
}

void WideLoadout::UpdateBomb(Player & /*player*/, EnemyManager &enemies,
                             EffectManager &effects, uint16_t remaining) {
  int dx = 0;
  int dy = 0;
  int l = 0;

  if (remaining > BombDuration() - 30) {
    return;
  }

  const auto d = static_cast<uint8_t>(remaining * 3U);
  l = (BombDuration() - remaining) * 26;
  const auto x_offset =
      math::RoundedPolarVector(math::AngleFromLegacy(d), l << 1);
  const auto y_offset =
      std::sin(static_cast<float>(d << 1) * math::kLegacyAngleStep) *
      static_cast<float>(l);
  dx = playfield::kWorldCenterX + 35_px + x_offset.x;
  dy = playfield::kWorldCenterY - 45_px +
       static_cast<int>(std::lround(y_offset));

  effects.SpawnFragment(dx, dy, FragmentKind::SmallStar);
  effects.SpawnFragment(dx, dy, FragmentKind::SmallStar);
  effects.SpawnFragment(dx, dy, FragmentKind::LargeStar);

  enemies.ApplyPlayerAttack(PlayerAttack::AllEnemies(1));
}

void WideLoadout::FireMainFocused(Player &player_, uint8_t tier) {
  switch (tier) {
  case 0:
  case 1: {
    PlayerShotSpawnInfo const si{.x = player_.X(),
                                 .y = player_.Y(),
                                 .direction = 192,
                                 .direction_step = 0,
                                 .count = 1,
                                 .speed = 13.5_px,
                                 .acceleration = 0,
                                 .kind = PlayerShotKind::WideFocusMain};
    player_.SpawnShot(si);
    break;
  }
  case 2: {
    PlayerShotSpawnInfo si{.x = player_.X() - 6_px,
                           .y = player_.Y(),
                           .direction = 192,
                           .direction_step = 0,
                           .count = 1,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideFocusMain};
    player_.SpawnShot(si);
    si.x += 12_px;
    player_.SpawnShot(si);
    break;
  }
  default: {
    const int count = (tier <= 5) ? 3 : 4;
    const int spread = (count - 1) * 12_px;
    PlayerShotSpawnInfo si{.x = player_.X() - spread / 2,
                           .y = player_.Y(),
                           .direction = 192,
                           .direction_step = 0,
                           .count = 1,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideFocusMain};
    for (int i = 0; i < count; i++) {
      player_.SpawnShot(si);
      si.x += 12_px;
    }
    break;
  }
  }
}

void WideLoadout::FireSubFocused(Player &player_, uint8_t tier) {
  switch (tier) {
  case 0:
    break;
  case 1:
  case 2:
  case 3: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 194,
                           .direction_step = 0,
                           .count = 1,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 190;
    player_.SpawnShot(si);
    break;
  }
  case 4:
  case 5: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 194,
                           .direction_step = 1,
                           .count = 2,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 190;
    player_.SpawnShot(si);
    break;
  }
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 195,
                           .direction_step = 2,
                           .count = 3,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 189;
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{.x = player_.OpX() +
                                PixelToWorld(OptionOffset(false)),
                           .y = player_.OpY(),
                           .direction = 196,
                           .direction_step = 2,
                           .count = 4,
                           .speed = 13.5_px,
                           .acceleration = 0,
                           .kind = PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 188;
    player_.SpawnShot(si);
    break;
  }
  }
}

void WideLoadout::DrawBombBackground(const Player & /*player*/,
                                     uint16_t remaining) const {
  static constexpr std::array<PixelLtrb, 6> frames = {
      PixelLtrb{0, 0, 210, 240},
      PixelLtrb{210, 0, 210 * 2, 240},
      PixelLtrb{210 * 2, 0, 210 * 3, 240},
      PixelLtrb{0, 240, 210, 480},
      PixelLtrb{210, 240, 210 * 2, 480},
      PixelLtrb{210 * 2, 240, 210 * 3, 480}};

  if (remaining == 0) {
    return;
  }

  int frame = 0;
  if (remaining > 80) {
    frame = std::clamp((BombDuration() - remaining) / 4, 0, 5);
  } else {
    frame = std::min<int>(remaining / 4, 5);
  }
  GraphicsSurfaceBlit({playfield::kLeft + 100, playfield::kTop + 100},
                      SurfaceId::Bomber, frames[frame]);
}
