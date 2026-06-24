///
/// WideForm implementation - spread shot (base) and straight columns (focus).
///

#include "wide_form.h"

#include "core/gian.h"
#include "core/world.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

// --- WideForm (base: spread shot) ---

void WideForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_WIDE_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64, 0)
                                      .spd(54, 0)
                                      .num(1, 0));
    break;
  case 1: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_WIDE_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64 + dd, 0)
                                      .spd(54, 0)
                                      .num(1, 0));
    break;
  }
  case 2: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    auto cmd = bullets::BulletCommand::way(TID_WIDE_MAIN)
                   .xy(player_.X() - (6 * 64), player_.Y())
                   .deg(-64 + dd, 0)
                   .spd(54, 0)
                   .num(1, 0);
    player_.Bullets().SpawnPlayer(cmd);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    break;
  }
  case 3:
  case 4:
  case 5: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_WIDE_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64 + dd, 4)
                                      .spd(54, 0)
                                      .num(3, 0));
    break;
  }
  default: {
    // tier 6-8: 5-way
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    player_.Bullets().SpawnPlayer(bullets::BulletCommand::way(TID_WIDE_MAIN)
                                      .xy(player_.X(), player_.Y())
                                      .deg(-64 + dd, 3)
                                      .spd(54, 0)
                                      .num(5, 0));
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
  case 3:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 5, 0)
            .spd(54, 0)
            .num(1, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 5, 0)
            .spd(54, 0)
            .num(1, 0));
    break;
  case 4:
  case 5:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 8, 7)
            .spd(54, 0)
            .num(2, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 8, 7)
            .spd(54, 0)
            .num(2, 0));
    break;
  case 6:
  case 7:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 10, 8)
            .spd(54, 0)
            .num(3, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 10, 8)
            .spd(54, 0)
            .num(3, 0));
    break;
  default:
    // tier 8
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 12, 8)
            .spd(54, 0)
            .num(4, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 12, 8)
            .spd(54, 0)
            .num(4, 0));
    break;
  }
}

void WideForm::FireBomb() {
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

  Enemies.DamageAll(1);
}

uint16_t WideForm::BombDuration() const { return WIDE_BOMB_TIME; }

// --- WideFocusForm (focus: straight parallel columns) ---

void WideFocusForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
  case 1:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_MAIN)
            .xy(player_.X(), player_.Y())
            .deg(-64, 0)
            .spd(54, 0)
            .num(1, 0));
    break;
  case 2: {
    auto cmd = bullets::BulletCommand::way(TID_WIDE_FOCUS_MAIN)
                   .deg(-64, 0)
                   .spd(54, 0)
                   .num(1, 0)
                   .xy(player_.X() - (6 * 64), player_.Y());
    player_.Bullets().SpawnPlayer(cmd);
    cmd.x += (12 * 64);
    player_.Bullets().SpawnPlayer(cmd);
    break;
  }
  default: {
    // tier 3-5: 3 columns, tier 6-8: 4 columns
    const int count = (tier <= 5) ? 3 : 4;
    const int spread = (count - 1) * (12 * 64);
    auto cmd = bullets::BulletCommand::way(TID_WIDE_FOCUS_MAIN)
                   .deg(-64, 0)
                   .spd(54, 0)
                   .num(1, 0)
                   .xy(player_.X() - spread / 2, player_.Y());
    for (int i = 0; i < count; i++) {
      player_.Bullets().SpawnPlayer(cmd);
      cmd.x += (12 * 64);
    }
  } break;
  }
}

void WideFocusForm::FireSub(uint8_t tier) {
  switch (tier) {
  case 0:
    break;
  case 1:
  case 2:
  case 3:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 2, 0)
            .spd(54, 0)
            .num(1, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 2, 0)
            .spd(54, 0)
            .num(1, 0));
    break;
  case 4:
  case 5:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 2, 1)
            .spd(54, 0)
            .num(2, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 2, 1)
            .spd(54, 0)
            .num(2, 0));
    break;
  case 6:
  case 7:
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 3, 2)
            .spd(54, 0)
            .num(3, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 3, 2)
            .spd(54, 0)
            .num(3, 0));
    break;
  default:
    // tier 8
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() + (SBOPT_DX * 64), player_.OpY())
            .deg(-64 + 4, 2)
            .spd(54, 0)
            .num(4, 0));
    player_.Bullets().SpawnPlayer(
        bullets::BulletCommand::way(TID_WIDE_FOCUS_SUB)
            .xy(player_.OpX() - (SBOPT_DX * 64), player_.OpY())
            .deg(-64 - 4, 2)
            .spd(54, 0)
            .num(4, 0));
    break;
  }
}