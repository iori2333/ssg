///
/// HomingForm implementation - tracking shots (base) and straight
/// high-power columns (focus).
///

#include "homing_form.h"

#include "game/cast.h"
#include "game/ut_math.h"
#include "gian.h"
#include "player.h"

// --- HomingForm (base: tracking sub-shots) ---

void HomingForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0: {
    player_.toge_ex += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex, 4));
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(player_.x, player_.y);
    TamaSetDeg(-64 + dd, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    break;
  }
  case 1:
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(player_.x - (6 * 64), player_.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    break;
  case 2:
  case 3:
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(player_.x, player_.y);
    TamaSetDeg(-64, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    player_.SpawnShot_();
    break;
  default:
    // tier 4-8: 5-way
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(player_.x, player_.y);
    TamaSetDeg(-64, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    player_.SpawnShot_();
    break;
  }
}

void HomingForm::FireSub(uint8_t tier) {
  if (tier == 0) {
    return;
  }

  TamaSTDForm(TID_HOMING_SUB);
  Bullets.command.type = T_SBHOMING;
  Bullets.command.rep = 64;
  Bullets.command.vd = 5;

  // Right option
  TamaSetXY(player_.opx + (SBOPT_DX * 64), player_.opy);
  TamaSetSpd(28, 4);
  if (tier < 8) {
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
  } else {
    TamaSetDeg(64 - 22, 30);
    TamaSetNum(2, 0);
  }
  player_.SpawnShot_();

  // Left option
  TamaSetXY(player_.opx - (SBOPT_DX * 64), player_.opy);
  if (tier < 8) {
    TamaSetDeg(64 + 5, 0);
  } else {
    TamaSetDeg(64 + 22, 30);
  }
  player_.SpawnShot_();
}

void HomingForm::FireBomb() {
  if (player_.bomb_time % 30 == 1) {
    TamaSTDForm(TID_HOMING_BOMB_A);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(player_.x, player_.y);
    TamaSetSpd(28, 4);
    TamaSetDeg(64, 16);
    TamaSetNum(8, 1);
    player_.SpawnShot_();
  }
}

uint16_t HomingForm::BombDuration() const { return HOMING_BOMB_TIME; }

// --- HomingFocusForm (focus: straight columns, no tracking) ---

void HomingFocusForm::FireMain(uint8_t tier) {
  const int count = (tier <= 0) ? 1 : (tier <= 2) ? 2 : (tier <= 4) ? 3 : 4;
  const int spread = (count - 1) * (12 * 64);

  TamaSTDForm(TID_HOMING_FOCUS_MAIN);
  TamaSetDeg(-64, 0);
  TamaSetSpd(54, 0);
  TamaSetNum(1, 0);
  TamaSetXY(player_.x - spread / 2, player_.y);
  for (int i = 0; i < count; i++) {
    player_.SpawnShot_();
    Bullets.command.x += (12 * 64);
  }
}

void HomingFocusForm::FireSub(uint8_t tier) {
  if (tier == 0) {
    return;
  }

  TamaSTDForm(TID_HOMING_FOCUS_SUB);
  TamaSetDeg(-64, 0);
  TamaSetSpd(54, 0);
  TamaSetNum(1, 0);

  // Right option
  TamaSetXY(player_.opx + (SBOPT_DX * 64), player_.opy);
  player_.SpawnShot_();

  // Left option
  TamaSetXY(player_.opx - (SBOPT_DX * 64), player_.opy);
  player_.SpawnShot_();
}
