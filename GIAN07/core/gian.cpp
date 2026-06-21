///
/// Gian - Game-wide management
///

#include <format>

#include "config.h"
#include "gian.h"
#include "level.h"

#include "bullet/bullet_manager.h"
#include "bullet/laser_manager.h"
#include "effect/font_uty.h"
#include "enemy/enemy_manager.h"
#include "gameflow/rank_manager.h"
#include "player/item_manager.h"
#include "player/player.h"
#include "stage/scroll_manager.h"
#include "util/time.h"

// [Global variables]
// HIGH_SCORE		*HighScore;
// char			ScoreTable[8][80];
// Games.game_count, Games.game_stage, Games.game_level →
// Moved to GameManager in game_manager.cpp; Viv moved to
// PlayerManager in player_manager.cpp (declared as Player& in MAID.h)

// [Functions (private)]
void StdStatusOutput() {
  const WINDOW_COORD column2_left = (GRP_RES.w - 128);

  static uint32_t prev;
  static uint32_t fps;
  static uint32_t count;
  // extern InputConfig			IConfig;
  const char *const DItem[4] = {"Easy", "Norm", "Hard", "Luna"};

#ifdef PBG_DEBUG
  if (!DebugDat.MsgDisplay)
    return;
#endif

  const auto now = Time_SteadyTicksMS();
  if ((now - prev) <= 1000) {
    count++;
  } else {
    fps = count;
    count = 0;
    prev = now;
  }

  GrpPut16(0, 0, std::format("{:03} FPS", fps).c_str());

  // ---- RANK display ----
  const char *const DiffName[5] = {"Easy", "Normal", "Hard", "Lunatic",
                                   "Extra"};
  const auto lv = (Games.game_stage == GRAPH_ID_EXSTAGE)
                      ? std::to_underlying(GameLevel::EXTRA)
                      : std::to_underlying(Games.game_level);

  GrpPut16(0, 34, std::format("RK  {:5}", Ranking.state.Rank).c_str());
  GrpPut16(0, 50,
           std::format("LV{:>7}", (lv < 5) ? DiffName[lv] : "????").c_str());
  GrpPut16(0, 82, std::format("Miss{:5}", Players.MissCount()).c_str());
  GrpPut16(0, 98, std::format("Bomb{:5}", Players.BombUsed()).c_str());
  GrpPut16(0, 114, std::format("DthB{:5}", Players.DeathbombCount()).c_str());
  GrpPut16(0, 146, "Stars");

  const auto capped = std::min(Players.StarCounter(), 9999U);
  GrpPut16(0, 162,
           std::format("{:4}/{:4}", capped, Players.StarThreshold()).c_str());

#ifdef PBG_DEBUG
  // sprintf(buf,"%s",DItems.entities[ConfigDat.Games.game_level.v]);
  // GrpPut16(0,50,buf);

  GrpPut16(0, 96 + 40, std::format("Enemy {:3}", Enemies.count).c_str());

  GrpPut16(0, 128 + 40, std::format("Tama1 {:3}", Bullets.count_small).c_str());
  GrpPut16(0, 148 + 40, std::format("Tama2 {:3}", Bullets.count_large).c_str());
  GrpPut16(0, 176 + 40, std::format("Laser {:3}", Lasers.count).c_str());
  GrpPut16(0, 196 + 40,
           std::format("HLaser {:2}", Lasers.homing_count).c_str());

  GrpPut16(0, 224 + 40, std::format("MTama {:3}", Players.ShotCount()).c_str());

  GrpPut16(0, 252 + 40, std::format("Item  {:3}", Items.count).c_str());

  GrpPut16(0, 290 + 40, std::format("Pow   {:3}", Players.Power()).c_str());

  GrpPut16(0, 320 + 40,
           std::format("SSPD  {:3}", Scroller.scroll.ScrollSpeed).c_str());

  GrpPut16(0, 440, "Gian07");
  GrpPut16(0, 460, "DebugMode");

  GrpPut16(column2_left, 100, "SCL Count");
  GrpPut16(column2_left, 120,
           std::format(" {:5}", Games.game_count).c_str());
#else
  // GrpPut16(0,440,"G07");
  // GrpPut16(0,460,"12/5 Ver");
#endif

  const auto tm = Time_NowLocal();

  GrpPut16(column2_left, 0, "Date");
  GrpPut16(column2_left, 20,
           std::format("{:02}/{:02}/{:02}", tm.month, tm.day, (tm.year % 100U))
               .c_str());

  GrpPut16(column2_left, 50, "Time");
  GrpPut16(
      column2_left, 70,
      std::format("{:02}:{:02}:{:02}", tm.hour, tm.minute, tm.second).c_str());

#ifndef PBG_DEBUG // pbg quirk
  GrpPut16(column2_left, 400,
           std::format("Bomb   {}", Players.Bombs()).c_str());
#endif

  GrpPut16(column2_left, 440,
           std::format("Left   {}", Players.Lives()).c_str());
  GrpPut16(column2_left, 460,
           std::format("Credit {}", Players.Credits()).c_str()); // Beware of -1
}
