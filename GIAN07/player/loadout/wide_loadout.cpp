///
/// WideLoadout - spread shot and focused straight columns.
///

#include <algorithm>
#include <cmath>

#include "wide_loadout.h"

#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/playfield.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "player/player_attack.h"
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
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), 192, 0,
        1,           13.5_px,     0,   PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  case 1: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0f)));
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), static_cast<uint8_t>(-64 + dd), 0, 1,
        13.5_px,     0,           PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  case 2: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0f)));
    PlayerShotSpawnInfo si{player_.X() - 6_px,
                           player_.Y(),
                           static_cast<uint8_t>(-64 + dd),
                           0,
                           1,
                           13.5_px,
                           0,
                           PlayerShotKind::WideMain};
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
        6.0f)));
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), static_cast<uint8_t>(-64 + dd), 4, 3,
        13.5_px,     0,           PlayerShotKind::WideMain};
    player_.SpawnShot(si);
    break;
  }
  default: {
    shot_phase_ += 32;
    const auto dd = static_cast<int8_t>(static_cast<int>(std::lround(
        std::sin(static_cast<float>(shot_phase_) * math::kLegacyAngleStep) *
        6.0f)));
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), static_cast<uint8_t>(-64 + dd), 3, 5,
        13.5_px,     0,           PlayerShotKind::WideMain};
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
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           197,
                           0,
                           1,
                           13.5_px,
                           0,
                           PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 187;
    player_.SpawnShot(si);
    break;
  }
  case 4:
  case 5: {
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           200,
                           7,
                           2,
                           13.5_px,
                           0,
                           PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 200 - 16;
    player_.SpawnShot(si);
    break;
  }
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           202,
                           8,
                           3,
                           13.5_px,
                           0,
                           PlayerShotKind::WideSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 202 - 20;
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           204,
                           8,
                           4,
                           13.5_px,
                           0,
                           PlayerShotKind::WideSub};
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
  int dx = 0, dy = 0, l = 0;

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
    PlayerShotSpawnInfo si{
        player_.X(), player_.Y(), 192, 0,
        1,           13.5_px,     0,   PlayerShotKind::WideFocusMain};
    player_.SpawnShot(si);
    break;
  }
  case 2: {
    PlayerShotSpawnInfo si{
        player_.X() - 6_px,           player_.Y(), 192, 0, 1, 13.5_px, 0,
        PlayerShotKind::WideFocusMain};
    player_.SpawnShot(si);
    si.x += 12_px;
    player_.SpawnShot(si);
    break;
  }
  default: {
    const int count = (tier <= 5) ? 3 : 4;
    const int spread = (count - 1) * 12_px;
    PlayerShotSpawnInfo si{
        player_.X() - spread / 2,     player_.Y(), 192, 0, 1, 13.5_px, 0,
        PlayerShotKind::WideFocusMain};
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
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           194,
                           0,
                           1,
                           13.5_px,
                           0,
                           PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 190;
    player_.SpawnShot(si);
    break;
  }
  case 4:
  case 5: {
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           194,
                           1,
                           2,
                           13.5_px,
                           0,
                           PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 190;
    player_.SpawnShot(si);
    break;
  }
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           195,
                           2,
                           3,
                           13.5_px,
                           0,
                           PlayerShotKind::WideFocusSub};
    player_.SpawnShot(si);
    si.x = player_.OpX() - PixelToWorld(OptionOffset(false));
    si.direction = 189;
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.OpX() + PixelToWorld(OptionOffset(false)),
                           player_.OpY(),
                           196,
                           2,
                           4,
                           13.5_px,
                           0,
                           PlayerShotKind::WideFocusSub};
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
  static constexpr PixelLtrb frames[6] = {
      {0, 0, 210, 240},           {210, 0, 210 * 2, 240},
      {210 * 2, 0, 210 * 3, 240}, {0, 240, 210, 480},
      {210, 240, 210 * 2, 480},   {210 * 2, 240, 210 * 3, 480}};

  if (remaining == 0) {
    return;
  }

  int frame;
  if (remaining > 80) {
    frame = std::clamp((BombDuration() - remaining) / 4, 0, 5);
  } else {
    frame = std::min<int>(remaining / 4, 5);
  }
  GraphicsSurfaceBlit({playfield::kLeft + 100, playfield::kTop + 100},
                      SurfaceId::Bomber, frames[frame]);
}
