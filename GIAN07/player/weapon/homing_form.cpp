///
/// HomingForm implementation - tracking shots (base) and straight
/// high-power columns (focus).
///

#include "homing_form.h"

#include "core/gian.h"
#include "enemy/enemy_manager.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

// --- HomingForm (base: tracking sub-shots) ---

void HomingForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 4));
    PlayerShotSpawnInfo si{player_.X(), player_.Y(),
                          static_cast<uint8_t>(-64 + dd), 0, 1,
                          SPEEDM(54), 0, TID_HOMING_MAIN};
    player_.SpawnShot(si);
    break;
  }
  case 1: {
    PlayerShotSpawnInfo si{player_.X() - (6 * 64), player_.Y(),
                          192, 0, 1, SPEEDM(54), 0, TID_HOMING_MAIN};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    player_.SpawnShot(si);
    break;
  }
  case 2:
  case 3: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 7, 3,
                          SPEEDM(54), 0, TID_HOMING_MAIN};
    player_.SpawnShot(si);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 7, 5,
                          SPEEDM(54), 0, TID_HOMING_MAIN};
    player_.SpawnShot(si);
    break;
  }
  }
}

void HomingForm::FireSub(uint8_t tier) {
  if (tier == 0) {
    return;
  }

  PlayerShotSpawnInfo si{};
  si.type = 9;     // T_SBHOMING
  si.rep = 64;
  si.vd = 5;
  si.v = SPEEDM(28);
  si.a = 4;
  si.c = TID_HOMING_SUB;

  si.x = player_.OpX() + (SBOPT_DX * 64);
  si.y = player_.OpY();
  if (tier < 8) {
    si.d = 59;
    si.dw = 0;
    si.n = 1;
  } else {
    si.d = 42;
    si.dw = 30;
    si.n = 2;
  }
  player_.SpawnShot(si);

  si.x = player_.OpX() - (SBOPT_DX * 64);
  if (tier < 8) {
    si.d = 69;
    si.dw = 0;
  } else {
    si.d = 86;
    si.dw = 30;
  }
  player_.SpawnShot(si);
}

void HomingForm::FireBomb() {
  if (player_.bomb_time_ % 30 == 1) {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 64, 16, 8,
                          SPEEDM(28), 4, TID_HOMING_BOMB_A, 9, 64, 5};
    player_.SpawnShot(si);
  }
}

uint16_t HomingForm::BombDuration() const { return HOMING_BOMB_TIME; }

// --- HomingFocusForm (focus: straight columns, no tracking) ---

void HomingFocusForm::FireMain(uint8_t tier) {
  const int count = (tier <= 0) ? 1 : (tier <= 2) ? 2 : (tier <= 4) ? 3 : 4;
  const int spread = (count - 1) * (12 * 64);

  PlayerShotSpawnInfo si{player_.X() - spread / 2, player_.Y(),
                        192, 0, 1, SPEEDM(54), 0, TID_HOMING_FOCUS_MAIN};
  for (int i = 0; i < count; i++) {
    player_.SpawnShot(si);
    si.x += 12 * 64;
  }
}

void HomingFocusForm::FireSub(uint8_t tier) {
  if (tier == 0) {
    return;
  }

  PlayerShotSpawnInfo si{player_.OpX() + (SBOPT_DX * 64), player_.OpY(),
                        192, 0, 1, SPEEDM(54), 0, TID_HOMING_FOCUS_SUB};
  player_.SpawnShot(si);

  si.x = player_.OpX() - (SBOPT_DX * 64);
  player_.SpawnShot(si);
}
