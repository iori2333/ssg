/*                                                                           */
/*   GIAN.cpp   ゲーム全体の管理                                             */
/*                                                                           */
/*                                                                           */

#include "config.h"
#include "font_uty.h"
#include "gian.h"
#include "level.h"
#include "bullet/bullet_manager.h"
#include "bullet/laser_manager.h"
#include "enemy/enemy_manager.h"
#include "gameflow/rank_manager.h"
#include "player/item_manager.h"
#include "player/player_manager.h"
#include "stage/scroll_manager.h"
#include "platform/time.h"

///// [グローバル変数] /////
// HIGH_SCORE		*HighScore;
// char			ScoreTable[8][80];
// GameState.game_count, GameState.game_stage, GameState.game_level → game_manager.cpp の GameManager に移動
// Viv → player_manager.cpp の PlayerManager に移動（MAID.h で Player& として宣言）

///// [ 関数(非公開) ] /////
void StdStatusOutput() {
  const WINDOW_COORD column2_left = (GRP_RES.w - 128);

  static uint32_t prev;
  static uint32_t fps, count;
  // extern InputConfig			IConfig;
  const char *const DItem[4] = {"Easy", "Norm", "Hard", "Luna"};
  char buf[100];

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

  sprintf(buf, "%03u FPS", fps);
  GrpPut16(0, 0, buf);

  // ---- RANK 表示 ----
  {
    const char *const DiffName[5] = {"Easy", "Normal", "Hard", "Lunatic",
                                     "Extra"};
    const auto lv = (GameState.game_stage == GRAPH_ID_EXSTAGE) ? GAME_EXTRA : GameState.game_level;

    sprintf(buf, "RK  %5d", Ranking.state.Rank);
    GrpPut16(0, 34, buf);
    sprintf(buf, "LV%7s", (lv < 5) ? DiffName[lv] : "????");
    GrpPut16(0, 50, buf);
    sprintf(buf, "Miss%5d", Players.viv.miss_count);
    GrpPut16(0, 82, buf);
    sprintf(buf, "Bomb%5d", Players.viv.bomb_used);
    GrpPut16(0, 98, buf);
  }

#ifdef PBG_DEBUG
#ifdef SUPPORT_GRP_BITDEPTH
  sprintf(buf, "%2dBppMode", ConfigDat.BitDepth.v.value());
  GrpPut16(0, 32, buf);
#endif
  // sprintf(buf,"%s",DItems.entities[ConfigDat.GameState.game_level.v]);
  // GrpPut16(0,50,buf);

  sprintf(buf, "Enemy %3d", Enemies.count);
  GrpPut16(0, 96 + 40, buf);

  sprintf(buf, "Tama1 %3d", Bullets.count_small);
  GrpPut16(0, 128 + 40, buf);
  sprintf(buf, "Tama2 %3d", Bullets.count_large);
  GrpPut16(0, 148 + 40, buf);
  sprintf(buf, "Laser %3d", Lasers.count);
  GrpPut16(0, 176 + 40, buf);
  sprintf(buf, "HLaser %2d", Lasers.homing_count);
  GrpPut16(0, 196 + 40, buf);

  sprintf(buf, "MTama %3d", Players.maid_tama_now);
  GrpPut16(0, 224 + 40, buf);

  sprintf(buf, "Item  %3d", Items.count);
  GrpPut16(0, 252 + 40, buf);

  sprintf(buf, "Pow   %3d", Players.viv.exp);
  GrpPut16(0, 290 + 40, buf);

  sprintf(buf, "SSPD  %3d", Scroller.scroll.ScrollSpeed);
  GrpPut16(0, 320 + 40, buf);

  GrpPut16(0, 440, "Gian07");
  GrpPut16(0, 460, "DebugMode");

  GrpPut16(column2_left, 100, "SCL Count");
  sprintf(buf, " %5d", GameState.game_count);
  GrpPut16(column2_left, 120, buf);
#else
  // GrpPut16(0,440,"G07");
  // GrpPut16(0,460,"12/5 Ver");
#endif

  const auto tm = Time_NowLocal();

  sprintf(buf, "%02u/%02u/%02u", tm.month, tm.day, (tm.year % 100u));
  GrpPut16(column2_left, 0, "Date");
  GrpPut16(column2_left, 20, buf);

  sprintf(buf, "%02u:%02u:%02u", tm.hour, tm.minute, tm.second);
  GrpPut16(column2_left, 50, "Time");
  GrpPut16(column2_left, 70, buf);

#ifndef PBG_DEBUG // pbg quirk
  sprintf(buf, "Bomb   %d", Players.viv.bomb);
  GrpPut16(column2_left, 400, buf);
#endif

  sprintf(buf, "Left   %d", Players.viv.left);
  GrpPut16(column2_left, 440, buf);
  sprintf(buf, "Credit %d", Players.viv.credit); // -1 に注意だ！！
  GrpPut16(column2_left, 460, buf);
}
