/*
 *   SCL Stage Command executor — extracted from SCROLL.cpp's enemy_set()
 */

#include "ecl/scl_executor.h"
#include "ecl/SCL.h"
#include "effect/EFFECT3D.h"
#include "enemy/BOSS.h"
#include "enemy/ENEMY.h"
#include "enemy/enemy_manager.h"
#include "entity/MAID.h"
#include "game/CONFIG.h"
#include "game/DEMOPLAY.h"
#include "game/ENDING.h"
#include "game/GAMEMAIN.h"
#include "game/GIAN.h"
#include "game/LEVEL.h"
#include "game/LOADER.h"
#include "game/MUSIC.h"
#include "game/PRankCtrl.h"
#include "game/SCROLL.h"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/endian.h"
#include "game/input.h"
#include "game/snd.h"
#include "platform/graphics_backend.h"
#include "ui/WindowCtrl.h"
#include "ui/WindowSys.h"

// SCL debug macro (was in SCROLL.cpp)
static void SCL_DEBUG(std::u8string_view s) {
#ifdef SCRIPT_TRACE
  DebugLog(s);
#endif
}

// Places a single enemy from SCL_ENEMY data (was static in SCROLL.cpp)
// Uses Enemies (EnemyManager) for entity allocation
static void _PutEnemy(const uint8_t *p) {
  if (Enemies.count + 1 >= ENEMY_MAX)
    return;

  auto *e = &Enemies.entities[Enemies.indices[Enemies.count++]];

  const uint32_t n = (4 + (p[4] << 2));

  const auto x = I16LEAt(&p[0]);
  const auto y = I16LEAt(&p[2]);
  InitEnemyDataSTD(e, x, y, n);
}

bool SclExecute(uint8_t *&scl_now) {
  bool bFlag = true;
  bool CtrlFlag = false;

  while (bFlag) {
    const auto *cmd = scl_now;
    switch (cmd[0]) {
    case (SCL_KEY): // キー入力待ち
      if ((Key_Data & (KEY_TAMA | KEY_RETURN | KEY_BOMB)) ||
          ++SclKeyWaitCount >= 180) {
        scl_now++;
        SclKeyWaitCount = 0;
      } else {
        bFlag = false;
      }
      break;

    case (SCL_TIME): {
      const auto temp = U32LEAt(&cmd[1]);
      // メッセージウィンドウがオープンしている場合 //
      if (SclInfo.MsgFlag) {
        if ((Key_Data & KEY_TAMA) || (Key_Data & KEY_RETURN) ||
            (Key_Data & KEY_BOMB)) {
          if (!SclInfo.ReturnFlag) {
            GameCount = temp;
            SclInfo.ReturnFlag = true;
          }
        } else {
          SclInfo.ReturnFlag = false;
        }
      }
      if (temp > GameCount) {
        bFlag = false;
      } else {
        scl_now += 5; // cmd(1)+time(4)
      }
      SCL_DEBUG(u8"--- SCL_TIME ---");
    } break;

    case (SCL_ENEMY):
      if (BossNow == 0)
        _PutEnemy(cmd + 1); // ボス出現中は出て来ちゃダメ
      scl_now += 6;         // cmd(1)+x(2)+y(2)+id(1)
      SCL_DEBUG(u8"--- SCL_ENEMY ---");
      break;

    case (SCL_BOSS): { // ボスをセットする(X(16),Y(16),ID(8))
      const auto x = I16LEAt(&cmd[1 + 0]); // ボス初期Ｘ
      const auto y = I16LEAt(&cmd[1 + 2]); // ボス初期Ｙ
      const auto id = cmd[1 + 2 + 2];      // ボスＩＤ
      BossSet(x, y, id);
      scl_now += (1 + 2 + 2 + 1); // cmd+x+y+id
    } break;

    case (SCL_BOSSDEAD): // ボスを強制的に破壊する(Level2 命令Only)
      BossKillAll();
      scl_now++;
      break;

    case (SCL_MWOPEN): // メッセージウィンドウを開く
      if (!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE)) {
        MWinOpen();
      }
      SclInfo.MsgFlag = true;
      scl_now++;
      break;

    case (SCL_MWCLOSE): // メッセージウィンドウを閉じる
      if (!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE)) {
        MWinClose();
      }
      SclInfo.MsgFlag = false;
      scl_now++;
      break;

    case (SCL_MSG): // メッセージを出力する
      MWinMsg(reinterpret_cast<const char *>(cmd + 1));
      scl_now += (strlen(reinterpret_cast<const char *>(cmd + 1)) + 2);
      break;

    case (SCL_FACE): // 顔を表示する
      MWinFace(cmd[1]);
      scl_now += 2;
      break;

    case (SCL_LOADFACE): // 顔グラをロードする(SurfaceID,FileNo)
      LoadFace(cmd[1], cmd[2]);
      scl_now += 3;
      break;

    case (SCL_NPG): // 新しいページに変更する
      MWinCmd(MWCMD_NEWPAGE);
      scl_now++;
      break;

    case (SCL_END): // カウントも変更させずにリターンする
      return true;

    case (SCL_SSP): // スクロールスピード変更
      ScrollSpeed(I16LEAt(&cmd[1]));
      scl_now += 3;
      break;

    case (SCL_MUSIC):
      if (!IsDemoplay) {
        BGM_Stop();
        if (BGM_Switch(cmd[1])) {
          BGM_Play();
          const auto mtitle = BGM_Title();
          if (!mtitle.empty()) {
            SetMusicTitle(460, mtitle);
          }
        }
      }
      scl_now += 2;
      break;

    case (SCL_DELENEMY):
      enemyind_set();
      scl_now++;
      break;

    case (SCL_EFC):
      switch (cmd[1]) {
      case (SEFC_WARN):
        Snd_SEPlay(8, GX_MID, true);
        WarningEffectSet();
        break;
      case (SEFC_WARNSTOP):
        Snd_SEStop(8);
        break;
      case (SEFC_MUSICFADE):
        BGM_FadeOut(120);
        break;
      case (SEFC_STG2BOSS):
        ScrollCommand(SCMD_STG2BOSS);
        break;
      case (SEFC_RASTERON):
        ScrollCommand(SCMD_RASTER_ON);
        break;
      case (SEFC_RASTEROFF):
        ScrollCommand(SCMD_RASTER_OFF);
        break;
      case (SEFC_STG3BOSS):
        ScrollCommand(SCMD_STG3BOSS);
        break;
      case (SEFC_STG3RESET):
        ScrollCommand(SCMD_STG3RESET);
        break;
      case (SEFC_CFADEIN):
        ScreenEffectSet(SCNEFC_CFADEIN);
        break;
      case (SEFC_CFADEOUT):
        ScreenEffectSet(SCNEFC_CFADEOUT);
        break;
      case (SEFC_STG6CUBE):
        ScrollCommand(SCMD_STG6CUBE);
        break;
      case (SEFC_STG6RNDECL):
        ScrollCommand(SCMD_STG6RNDECL);
        break;
      case (SEFC_STG4ROCK):
        ScrollCommand(SCMD_STG4ROCK);
        break;
      case (SEFC_STG4LEAVE):
        ScrollCommand(SCMD_STG4LEAVE);
        break;
      case (SEFC_WHITEIN):
        ScreenEffectSet(SCNEFC_WHITEIN);
        break;
      case (SEFC_WHITEOUT):
        ScreenEffectSet(SCNEFC_WHITEOUT);
        break;
      case (SEFC_LOADEX01):
        LoadGraph(GRAPH_ID_EXBOSS1);
        break;
      case (SEFC_LOADEX02):
        LoadGraph(GRAPH_ID_EXBOSS2);
        break;
      case (SEFC_STG6RASTER):
        ScrollCommand(SCMD_STG6RASTER);
        break;
      }
      scl_now += 2;
      break;

    case (SCL_WAITEX): // 特殊待ち <cmd1>,<opt4>
      switch (cmd[1]) {
      case (SWAIT_BOSSHP): // 残りＨＰ
        if (GetBossHPSum() <= U32LEAt(&cmd[2])) {
          break;
        }
        return false;
      case (SWAIT_BOSSLEFT): // 残りボス数
        if (BossNow <= U32LEAt(&cmd[2])) {
          break;
        }
        return false;
      }
      scl_now += (1 + 1 + 4);
      break;

    case (SCL_STAGECLEAR): // ステージクリア
      if (DemoplaySaveAllEnable) {
        DemoplayFlushStage();
        GameNextStage();
        return true;
      }
      if (DemoplayLoadAllEnable) {
        if (GameStage < PlaybackMaxStage) {
          GameNextStage();
        }
        return true;
      }
      GameNextStage();
      return true;

    case (SCL_GAMECLEAR):
      if (DemoplaySaveAllEnable) {
        DemoplayFlushStage();
        DemoplaySaveReplayAll();
      }
      if (DemoplayLoadAllEnable)
        return true;

      if (GameStage == STAGE_MAX)
        GameStage = 7;
      if (GameLevel != GAME_EASY) {
        switch (Viv.weapon) {
        case (0):
          ConfigDat.ExtraStgFlags.v |= 1;
          break;
        case (1):
          ConfigDat.ExtraStgFlags.v |= 2;
          break;
        case (2):
          ConfigDat.ExtraStgFlags.v |= 4;
          break;
        }
      }
      ConfigSave();
      EndingInit();
      return true;

    case (SCL_EXTRACLEAR):
      if (DemoplaySaveAllEnable) {
        DemoplayFlushStage();
        DemoplaySaveReplayAll(true);
      }
      if (DemoplayLoadAllEnable)
        return true;

      NameRegistInit(true);
      return true;

    case (SCL_MAPPALETTE):
      GrpSurface_PaletteApplyToBackend(SURFACE_ID::MAPCHIP);
      scl_now++;
      break;

    case (SCL_ENEMYPALETTE):
      LoadPaletteFromEnemy();
      scl_now++;
      break;

    default:
      MWinOpen();
      MWinCmd(MWCMD_NEWPAGE);
      MWinCmd(MWCMD_LARGEFONT);
      MWinMsg("バグ発生だにょ");
      SCL_DEBUG(u8"---- SCL !BUG! ---");
      return false;
    }
  }

  // Advance stage timer and update play rank (was at end of enemy_set())
  GameCount++;
  if ((GameCount & 0x3f) == 0) {
    if (GameStage == GRAPH_ID_EXSTAGE)
      PlayRankAdd(1);
    else
      PlayRankAdd(1 + GameStage / 3);
  }

  return false;
}
