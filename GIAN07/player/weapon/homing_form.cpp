///
/// HomingForm implementation - tracking shots (base) and straight
/// high-power columns (focus).
///

#include "homing_form.h"

#include "core/gian.h"
#include "core/world.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

void HomingForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 4));
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_HOMING_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64 + dd, 0)
                                      .spd(54, 0)
                                      .num(1, 0));
    break;
  }
  case 1: {
    auto cmd = bullets::BulletCommand::way(TID_HOMING_MAIN)
                   .xy(player_.X() - (6 * 64), player_.Y())
                   .deg(-64, 0)
                   .spd(54, 0)
                   .num(1, 0);
    player_.Bullets().SpawnPlayer(cmd);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    break;
  }
  case 2:
  case 3:
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_HOMING_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 7)
                                      .spd(54, 0)
                                      .num(3, 0));
    break;
  default:
    // tier 4-8: 5-way
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_HOMING_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 7)
                                      .spd(54, 0)
                                      .num(5, 0));
    break;
  }
}

void HomingForm::FireSub(uint8_t tier) {
  if (tier == 0) {
    return;
  }

  // Right option
  player_.Bullets().SpawnPlayer(
      bullets::BulletCommand::way(TID_HOMING_SUB)
          .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
          .spd(28, 4)
          .vel_type(T_SBHOMING)
          .homing(64, 5)
          .deg((tier < 8) ? (64 - 5) : (64 - 22), (tier < 8) ? 0 : 30)
          .num(1, 0));

  // Left option
  player_.Bullets().SpawnPlayer(
      bullets::BulletCommand::way(TID_HOMING_SUB)
          .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
          .spd(28, 4)
          .vel_type(T_SBHOMING)
          .homing(64, 5)
          .deg((tier < 8) ? (64 + 5) : (64 + 22), (tier < 8) ? 0 : 30)
          .num(1, 0));
}

void HomingForm::FireBomb() {
  if (player_.bomb_time_ % 30 == 1) {
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_HOMING_BOMB_A)
                                      .xy(player_.X(), player_.Y())
                                      .spd(28, 4)
                                      .vel_type(T_SBHOMING)
                                      .homing(64, 5)
                                      .deg(64, 16)
                                      .num(8, 1));
  }
}

uint16_t HomingForm::BombDuration() const { return HOMING_BOMB_TIME; }

// --- HomingFocusForm (focus: straight columns, no tracking) ---

void HomingFocusForm::FireMain(uint8_t tier) {
  const int count = (tier <= 0) ? 1 : (tier <= 2) ? 2 : (tier <= 4) ? 3 : 4;
  const int spread = (count - 1) * (12 * 64);

  auto cmd = bullets::BulletCommand::way(TID_HOMING_FOCUS_MAIN)
                 .deg(-64, 0)
                 .spd(54, 0)
                 .num(1, 0)
                 .xy(player_.X() - spread / 2, player_.Y());
  for (int i = 0; i < count; i++) {
    player_.Bullets().SpawnPlayer(cmd);
    cmd.x += (12 * 64);
  }
}

void HomingFocusForm::FireSub(uint8_t tier) {
  if (tier == 0) {
    return;
  }

  auto cmd = bullets::BulletCommand::way(TID_HOMING_FOCUS_SUB)
                 .deg(-64, 0)
                 .spd(54, 0)
                 .num(1, 0);

  // Right option
  cmd.xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
  player_.Bullets().SpawnPlayer(cmd);

  // Left option
  cmd.xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
  player_.Bullets().SpawnPlayer(cmd);
}