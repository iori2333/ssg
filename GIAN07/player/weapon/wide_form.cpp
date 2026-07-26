///
/// WideForm implementation - spread shot (base) and straight columns (focus).
///

#include "wide_form.h"

#include "core/gian.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_system.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

// --- WideForm (base: spread shot) ---

void WideForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 0,
                           1,           SPEEDM(54),  0,   TID_WIDE_MAIN};
    player_.SpawnShot(si);
    break;
  }
  case 1: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    PlayerShotSpawnInfo si{player_.X(),
                           player_.Y(),
                           static_cast<uint8_t>(-64 + dd),
                           0,
                           1,
                           SPEEDM(54),
                           0,
                           TID_WIDE_MAIN};
    player_.SpawnShot(si);
    break;
  }
  case 2: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    PlayerShotSpawnInfo si{player_.X() - (6 * 64),
                           player_.Y(),
                           static_cast<uint8_t>(-64 + dd),
                           0,
                           1,
                           SPEEDM(54),
                           0,
                           TID_WIDE_MAIN};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    player_.SpawnShot(si);
    break;
  }
  case 3:
  case 4:
  case 5: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    PlayerShotSpawnInfo si{player_.X(),
                           player_.Y(),
                           static_cast<uint8_t>(-64 + dd),
                           4,
                           3,
                           SPEEDM(54),
                           0,
                           TID_WIDE_MAIN};
    player_.SpawnShot(si);
    break;
  }
  default: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    PlayerShotSpawnInfo si{player_.X(),
                           player_.Y(),
                           static_cast<uint8_t>(-64 + dd),
                           3,
                           5,
                           SPEEDM(54),
                           0,
                           TID_WIDE_MAIN};
    player_.SpawnShot(si);
    break;
  }
  }
}

void WideForm::FireSub(uint8_t tier) {
  switch (tier) {
  case 0:
    break;
  case 1:
  case 2:
  case 3: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           197,
                           0,
                           1,
                           SPEEDM(54),
                           0,
                           TID_WIDE_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 187;
    player_.SpawnShot(si);
    break;
  }
  case 4:
  case 5: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           200,
                           7,
                           2,
                           SPEEDM(54),
                           0,
                           TID_WIDE_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 200 - 16;
    player_.SpawnShot(si);
    break;
  }
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           202,
                           8,
                           3,
                           SPEEDM(54),
                           0,
                           TID_WIDE_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 202 - 20;
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           204,
                           8,
                           4,
                           SPEEDM(54),
                           0,
                           TID_WIDE_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 204 - 24;
    player_.SpawnShot(si);
    break;
  }
  }
}

void WideForm::FireBomb(EnemySystem &enemies) {
  int dx = 0, dy = 0, l = 0;

  if (player_.bomb_time_ > WIDE_BOMB_TIME - 30) {
    return;
  }

  const auto d = Cast::down<uint8_t>(player_.bomb_time_ * 3U);
  l = (WIDE_BOMB_TIME - player_.bomb_time_) * 26;
  dx = GX_MID + (64 * 70 / 2) + cosl(d, l << 1);
  dy = GY_MID - (64 * 90 / 2) + sinl(d << 1, l);

  Effects.SpawnFragment(dx, dy, FRG_STAR1);
  Effects.SpawnFragment(dx, dy, FRG_STAR1);
  Effects.SpawnFragment(dx, dy, FRG_STAR2);

  enemies.ApplyAttack(EnemyAttack::All(1));
}

uint16_t WideForm::BombDuration() const { return WIDE_BOMB_TIME; }

// --- WideFocusForm (focus: straight parallel columns) ---

void WideFocusForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
  case 1: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 0,
                           1,           SPEEDM(54),  0,   TID_WIDE_FOCUS_MAIN};
    player_.SpawnShot(si);
    break;
  }
  case 2: {
    PlayerShotSpawnInfo si{
        player_.X() - (6 * 64), player_.Y(), 192, 0, 1, SPEEDM(54), 0,
        TID_WIDE_FOCUS_MAIN};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    player_.SpawnShot(si);
    break;
  }
  default: {
    const int count = (tier <= 5) ? 3 : 4;
    const int spread = (count - 1) * (12 * 64);
    PlayerShotSpawnInfo si{
        player_.X() - spread / 2, player_.Y(), 192, 0, 1, SPEEDM(54), 0,
        TID_WIDE_FOCUS_MAIN};
    for (int i = 0; i < count; i++) {
      player_.SpawnShot(si);
      si.x += 12 * 64;
    }
    break;
  }
  }
}

void WideFocusForm::FireSub(uint8_t tier) {
  switch (tier) {
  case 0:
    break;
  case 1:
  case 2:
  case 3: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           194,
                           0,
                           1,
                           SPEEDM(54),
                           0,
                           TID_WIDE_FOCUS_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 190;
    player_.SpawnShot(si);
    break;
  }
  case 4:
  case 5: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           194,
                           1,
                           2,
                           SPEEDM(54),
                           0,
                           TID_WIDE_FOCUS_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 190;
    player_.SpawnShot(si);
    break;
  }
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           195,
                           2,
                           3,
                           SPEEDM(54),
                           0,
                           TID_WIDE_FOCUS_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 189;
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64),
                           player_.OpY(),
                           196,
                           2,
                           4,
                           SPEEDM(54),
                           0,
                           TID_WIDE_FOCUS_SUB};
    player_.SpawnShot(si);
    si.x = player_.OpX() - (SBOPT_DX * 64);
    si.d = 188;
    player_.SpawnShot(si);
    break;
  }
  }
}
