///
/// LaserForm implementation - straight laser beams with narrowed
/// spread (focus).
///

#include "laser_form.h"

#include "gian.h"
#include "player.h"

// --- LaserForm (base: wider spread) ---

void LaserForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    break;
  case 1:
  case 2:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X() - (6 * 64), player_.Y());
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 50);
    break;
  case 3:
  case 4:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 6);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 100);
    break;
  case 5:
  case 6:
  case 7:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetSpd(54, 0);
    TamaSetDeg(-64 - 5, 10);
    TamaSetXY(player_.X() - (6 * 64), player_.Y());
    TamaSetNum(2, 0);
    player_.SpawnShot_();
    TamaSetDeg(-64 + 5, 10);
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 150);
    break;
  default:
    // tier 8
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 6);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 200);
    break;
  }
}

void LaserForm::FireBomb() {
  const auto LaserDeg = player_.GetLaserDeg();

  int ox = player_.OpX() + (SBOPT_DX * 64);
  int oy = player_.OpY();
  for (int i = -3; i <= 3; i++) {
    const auto d = Player::GetRightLaserDeg(LaserDeg, i);
    Enemies.DamageAt3(ox, oy, d);
  }

  ox = player_.OpX() - (SBOPT_DX * 64);
  oy = player_.OpY();
  for (int i = -3; i <= 3; i++) {
    const auto d = Player::GetLeftLaserDeg(LaserDeg, i);
    Enemies.DamageAt3(ox, oy, d);
  }
}

uint16_t LaserForm::BombDuration() const { return LASER_BOMB_TIME; }

void LaserForm::OnFireTick() {
  if (player_.lay_time_ != 0U) {
    player_.lay_time_--;
    if (player_.lay_time_ < 64) {
      player_.lay_grp_ = 0;
    } else if (player_.lay_time_ < 64 + 50) {
      player_.lay_grp_ = 1;
    } else if (player_.lay_time_ < 64 + 100) {
      player_.lay_grp_ = 2;
    } else if (player_.lay_time_ < 64 + 150) {
      player_.lay_grp_ = 3;
    } else {
      player_.lay_grp_ = 4;
    }
  }
}

// --- LaserFocusForm (focus: narrowed spread) ---

void LaserFocusForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    break;
  case 1:
  case 2:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X() - (6 * 64), player_.Y());
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 50);
    break;
  case 3:
  case 4:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 100);
    break;
  case 5:
  case 6:
  case 7:
    TamaSTDForm(TID_LASER_SUB);
    TamaSetSpd(54, 0);
    TamaSetDeg(-64 - 2, 4);
    TamaSetXY(player_.X() - (6 * 64), player_.Y());
    TamaSetNum(2, 0);
    player_.SpawnShot_();
    TamaSetDeg(-64 + 2, 4);
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 150);
    break;
  default:
    // tier 8
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    player_.SpawnShot_();
    Player::SetMLaser(64 + 200);
    break;
  }
}

void LaserFocusForm::OnFireTick() {
  if (player_.lay_time_ != 0U) {
    player_.lay_time_--;
    if (player_.lay_time_ < 64) {
      player_.lay_grp_ = 0;
    } else if (player_.lay_time_ < 64 + 50) {
      player_.lay_grp_ = 1;
    } else if (player_.lay_time_ < 64 + 100) {
      player_.lay_grp_ = 2;
    } else if (player_.lay_time_ < 64 + 150) {
      player_.lay_grp_ = 3;
    } else {
      player_.lay_grp_ = 4;
    }
  }
}
