///
/// Gian - Game-wide management
///

#include <algorithm>
#include <format>
#include <utility>

#include "config.h"
#include "gian.h"
#include "level.h"

#include "gfx/font_uty.h"
#include "gameflow/gameflow_manager.h"
#include "core/game_manager.h"
#include "player/player.h"
#include "util/time.h"

void StdStatusOutput() {
  const WINDOW_COORD column2_left = (GRP_RES.w - 128);

  static uint32_t prev;
  static uint32_t fps;
  static uint32_t count;
  // extern InputConfig			IConfig;
  const char *const DItem[4] = {"Easy", "Norm", "Hard", "Luna"};

  const auto now = Time_SteadyTicksMS();
  if ((now - prev) <= 1000) {
    count++;
  } else {
    fps = count;
    count = 0;
    prev = now;
  }

  GrpPut16(0, 0, std::format("FPS   {:3}", fps).c_str());

  // ---- RANK  display ----
  GrpPut16(0, 40, std::format("R {:7}", GameFlow.ctx.game.rank).c_str());
  // ---- LEVEL display ----
  auto lv = std::to_underlying(GameFlow.ctx.game.level);
  GrpPut16(0, 60,
           std::format("L {:>7}", lv < 5 ? LevelName[lv] : "Unknown").c_str());

  GrpPut16(0, 100, std::format("Miss {:4}", Players.MissCount()).c_str());
  GrpPut16(0, 120, std::format("Bomb {:4}", Players.BombUsed()).c_str());
  GrpPut16(0, 140, std::format("DthB {:4}", Players.DeathbombCount()).c_str());

  GrpPut16(0, 180, "Stars");
  auto stars = std::min(Players.StarCounter(), 9999U);
  auto threshold = std::min(Players.StarThreshold(), 9999U);
  GrpPut16(0, 200, std::format("{:4}/{:4}", stars, threshold).c_str());

  const auto tm = Time_NowLocal();
  GrpPut16(column2_left, 0, "Date");
  GrpPut16(column2_left, 20,
           std::format("{:02}/{:02}/{:02}", tm.month, tm.day, (tm.year % 100U))
               .c_str());

  GrpPut16(column2_left, 50, "Time");
  GrpPut16(
      column2_left, 70,
      std::format("{:02}:{:02}:{:02}", tm.hour, tm.minute, tm.second).c_str());

#ifdef PBG_DEBUG
  GrpPut16(column2_left, 360, "Debug  ON");
#endif

  if (ConfigDat.practice_mode == PracticeMode::AUTOBOMB) {
    GrpPut16(column2_left, 380, "Prac AUTO");
  } else if (ConfigDat.practice_mode == PracticeMode::INVINCIBLE) {
    GrpPut16(column2_left, 380, "Prac  INV");
  }

  GrpPut16(column2_left, 420,
           std::format("Bomb {:4}", Players.Bombs()).c_str());
  GrpPut16(column2_left, 440,
           std::format("Left {:4}", Players.Lives()).c_str());
  GrpPut16(column2_left, 460,
           std::format("Credit {:2}", Players.Credits()).c_str());
}
