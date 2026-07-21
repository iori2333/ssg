///
/// LaserForm implementation - straight laser beams with narrowed
/// spread (focus).
///

#include "laser_form.h"

#include "core/gian.h"
#include "enemy/enemy_manager.h"
#include "player/player.h"

// --- LaserForm (base: wider spread) ---

void LaserForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 0, 1,
                          SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    break;
  }
  case 1:
  case 2: {
    PlayerShotSpawnInfo si{player_.X() - (6 * 64), player_.Y(),
                          192, 0, 1, SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 50);
    break;
  }
  case 3:
  case 4: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 6, 3,
                          SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 100);
    break;
  }
  case 5:
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{player_.X() - (6 * 64), player_.Y(),
                          187, 10, 2, SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    si.d = 197;
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 150);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 6, 5,
                          SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 200);
    break;
  }
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

void LaserForm::OnCollisionTick() {
  if (player_.lay_grp_ != 0U) {
    const int ldmg = (player_.lay_grp_ / 3) + 1;
    Enemies.DamageAt2(player_.opx_ + (SBOPT_DX << 6), player_.opy_, ldmg);
    Enemies.DamageAt2(player_.opx_ - (SBOPT_DX << 6), player_.opy_, ldmg);
  }
}

// --- LaserFocusForm (focus: narrowed spread) ---

void LaserFocusForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 0, 1,
                          SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    break;
  }
  case 1:
  case 2: {
    PlayerShotSpawnInfo si{player_.X() - (6 * 64), player_.Y(),
                          192, 0, 1, SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 50);
    break;
  }
  case 3:
  case 4: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 2, 3,
                          SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 100);
    break;
  }
  case 5:
  case 6:
  case 7: {
    PlayerShotSpawnInfo si{player_.X() - (6 * 64), player_.Y(),
                          190, 4, 2, SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    si.x += 12 * 64;
    si.d = 194;
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 150);
    break;
  }
  default: {
    PlayerShotSpawnInfo si{player_.X(), player_.Y(), 192, 2, 4,
                          SPEEDM(54), 0, TID_LASER_SUB};
    player_.SpawnShot(si);
    player_.SetMLaser(64 + 200);
    break;
  }
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

void LaserFocusForm::OnCollisionTick() {
  if (player_.lay_grp_ != 0U) {
    const int loff = SBOPT_DX / 2;
    const int ldmg = (player_.lay_grp_ / 3) + 1;
    Enemies.DamageAt2(player_.opx_ + (loff << 6), player_.opy_, ldmg);
    Enemies.DamageAt2(player_.opx_ - (loff << 6), player_.opy_, ldmg);
  }
}
