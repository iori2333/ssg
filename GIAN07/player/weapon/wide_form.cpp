///
/// WideForm implementation - spread shot (base) and straight columns (focus).
///

#include "wide_form.h"

#include "game/cast.h"
#include "game/ut_math.h"
#include "gian.h"
#include "player.h"

// --- WideForm (base: spread shot) ---

void WideForm::FireMain(uint8_t tier) {
  switch (tier) {
  case 0:
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    break;
  case 1: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64 + dd, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    break;
  }
  case 2: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(player_.X() - (6 * 64), player_.Y());
    TamaSetDeg(-64 + dd, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    break;
  }
  case 3:
  case 4:
  case 5: {
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64 + dd, 4);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    player_.SpawnShot_();
    break;
  }
  default: {
    // tier 6-8: 5-way
    player_.toge_ex_ += 32;
    const auto dd = Cast::down<int8_t>(sinl(player_.toge_ex_, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64 + dd, 3);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    player_.SpawnShot_();
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
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 5, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 5, 0);
    player_.SpawnShot_();
    break;
  case 4:
  case 5:
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 8, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(2, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 8, 7);
    player_.SpawnShot_();
    break;
  case 6:
  case 7:
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 10, 8);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 10, 8);
    player_.SpawnShot_();
    break;
  default:
    // tier 8
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 12, 8);
    TamaSetSpd(54, 0);
    TamaSetNum(4, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 12, 8);
    player_.SpawnShot_();
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
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetXY(player_.X(), player_.Y());
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    break;
  case 2:
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(player_.X() - (6 * 64), player_.Y());
    player_.SpawnShot_();
    Bullets.command.x += (12 * 64);
    player_.SpawnShot_();
    break;
  default:
    // tier 3-5: 3 columns, tier 6-8: 4 columns
    {
      const int count = (tier <= 5) ? 3 : 4;
      const int spread = (count - 1) * (12 * 64);
      TamaSTDForm(TID_WIDE_FOCUS_MAIN);
      TamaSetDeg(-64, 0);
      TamaSetSpd(54, 0);
      TamaSetNum(1, 0);
      TamaSetXY(player_.X() - spread / 2, player_.Y());
      for (int i = 0; i < count; i++) {
        player_.SpawnShot_();
        Bullets.command.x += (12 * 64);
      }
    }
    break;
  }
}

void WideFocusForm::FireSub(uint8_t tier) {
  switch (tier) {
  case 0:
    break;
  case 1:
  case 2:
  case 3:
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 2, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 2, 0);
    player_.SpawnShot_();
    break;
  case 4:
  case 5:
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 2, 1);
    TamaSetSpd(54, 0);
    TamaSetNum(2, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 2, 1);
    player_.SpawnShot_();
    break;
  case 6:
  case 7:
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 3, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 3, 2);
    player_.SpawnShot_();
    break;
  default:
    // tier 8
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(player_.OpX() + (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 + 4, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(4, 0);
    player_.SpawnShot_();
    TamaSetXY(player_.OpX() - (SBOPT_DX * 64), player_.OpY());
    TamaSetDeg(-64 - 4, 2);
    player_.SpawnShot_();
    break;
  }
}
