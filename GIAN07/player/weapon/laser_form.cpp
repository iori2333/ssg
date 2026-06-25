///
/// LaserForm implementation - straight laser beams with narrowed
/// spread (focus).
///

#include "laser_form.h"

#include "core/gian.h"
#include "core/world.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"

namespace {

// Draws the infinite vertical continuous laser beam from `player`'s two
// option positions.  `loff` is the horizontal spacing between the two
// beams, in pixel units (base = `SBOPT_DX`, focus = `SBOPT_DX / 2`).
// The beam's visual intensity is selected by `lay_grp_`.
void DrawContinuousBeam(Player &player, int loff) {
  if (const auto grp = player.LayGrp(); grp == 0U) {
    return;
  } else {
    PIXEL_LTRB ltemp;
    int x = 0;
    int y = 0;

    // Beam heads near the options.
    ltemp = PIXEL_LTWH{(384 + ((grp - 1) << 4)), 240, 8, 16};
    x = (player.OpX() >> 6) + 4 - 8 + loff;
    y = (player.OpY() >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    x = (player.OpX() >> 6) + 4 - 8 - loff;
    y = (player.OpY() >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    // Beam tails continuously blitted above the heads.
    ltemp = PIXEL_LTWH{(384 + 8 + ((grp - 1) << 4)), 240, 8, 16};
    for (int i = (player.OpY() >> 6) - 36; i > -16; i -= 16) {
      x = (player.OpX() >> 6) + 4 - 8 + loff;
      GrpSurface_Blit({x, i}, SURFACE_ID::SYSTEM, ltemp);
    }
    for (int i = (player.OpY() >> 6) - 36; i > -16; i -= 16) {
      x = (player.OpX() >> 6) + 4 - 8 - loff;
      GrpSurface_Blit({x, i}, SURFACE_ID::SYSTEM, ltemp);
    }
  }
}

} // namespace

// --- LaserForm (base: wider spread) ---

void LaserForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_LASER_SUB)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 0)
                                      .spd(54, 0)
                                      .num(1, 0));
    break;
  case 1:
  case 2: {
    auto cmd = bullets::BulletCommand::way(TID_LASER_SUB)
                   .xy(player_.X() - (6 * 64), player_.Y())
                   .deg(-64, 0)
                   .spd(54, 0)
                   .num(1, 0);
    player_.Bullets().SpawnPlayer(cmd);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    Player::SetMLaser(64 + 50);
    break;
  }
  case 3:
  case 4:
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_LASER_SUB)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 6)
                                      .spd(54, 0)
                                      .num(3, 0));
    Player::SetMLaser(64 + 100);
    break;
  case 5:
  case 6:
  case 7: {
    auto cmd = bullets::BulletCommand::way(TID_LASER_SUB)
                   .spd(54, 0)
                   .deg(-64 - 5, 10)
                   .xy(player_.X() - (6 * 64), player_.Y())
                   .num(2, 0);
    player_.Bullets().SpawnPlayer(cmd);
    cmd.deg(-64 + 5, 10);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    Player::SetMLaser(64 + 150);
    break;
  }
  default:
    // tier 8
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_LASER_SUB)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 6)
                                      .spd(54, 0)
                                      .num(5, 0));
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
  case 0:
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_LASER_SUB)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 0)
                                      .spd(54, 0)
                                      .num(1, 0));
    break;
  case 1:
  case 2: {
    auto cmd = bullets::BulletCommand::way(TID_LASER_SUB)
                   .xy(player_.X() - (6 * 64), player_.Y())
                   .deg(-64, 0)
                   .spd(54, 0)
                   .num(1, 0);
    player_.Bullets().SpawnPlayer(cmd);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    Player::SetMLaser(64 + 50);
    break;
  }
  case 3:
  case 4:
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_LASER_SUB)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 2)
                                      .spd(54, 0)
                                      .num(3, 0));
    Player::SetMLaser(64 + 100);
    break;
  case 5:
  case 6:
  case 7: {
    auto cmd = bullets::BulletCommand::way(TID_LASER_SUB)
                   .spd(54, 0)
                   .deg(-64 - 2, 4)
                   .xy(player_.X() - (6 * 64), player_.Y())
                   .num(2, 0);
    player_.Bullets().SpawnPlayer(cmd);
    cmd.deg(-64 + 2, 4);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    Player::SetMLaser(64 + 150);
    break;
  }
  default:
    // tier 8: 4-way narrow spread
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_LASER_SUB)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 2)
                                      .spd(54, 0)
                                      .num(4, 0));
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

void LaserFocusForm::OnCollisionTick() {
  if (player_.lay_grp_ != 0U) {
    // Focus form: narrowed beam spacing, damage matches base.
    const int loff = SBOPT_DX / 2;
    const int ldmg = (player_.lay_grp_ / 3) + 1;
    Enemies.DamageAt2(player_.opx_ + (loff << 6), player_.opy_, ldmg);
    Enemies.DamageAt2(player_.opx_ - (loff << 6), player_.opy_, ldmg);
  }
}

// --- Continuous beam rendering ---

void LaserForm::DrawBeam() {
  // Base form: full beam spacing.
  DrawContinuousBeam(player_, SBOPT_DX);
}

void LaserFocusForm::DrawBeam() {
  // Focus form: beams pulled together.
  DrawContinuousBeam(player_, SBOPT_DX / 2);
}