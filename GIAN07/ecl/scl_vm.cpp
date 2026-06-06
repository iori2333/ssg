/*
 *   SclVM — Stage Control Language virtual machine
 */

#include "ecl/scl_vm.h"
#include "ecl/scl_commands.h"
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
#include "game/endian.h"
#include "game/input.h"
#include "game/snd.h"
#include "platform/graphics_backend.h"
#include "ui/WindowCtrl.h"
#include "ui/WindowSys.h"

// --- Global instance ---
std::optional<SclVM> SclVM::s_instance;

void SclVM::Init(const uint8_t *scl_data) {
  if (scl_data) {
    s_instance.emplace(scl_data);
  } else {
    s_instance.reset();
  }
}

void SclVM::Clear() { s_instance.reset(); }

SclVM &SclVM::Instance() { return *s_instance; }

bool SclVM::IsInitialized() { return s_instance.has_value(); }

// --- Spawn a single enemy from SCL_ENEMY data ---
void SclVM::SpawnEnemy(const SclCmdEnemy &c) {
  if (Enemies.count + 1 >= ENEMY_MAX)
    return;

  auto *e = &Enemies.entities[Enemies.indices[Enemies.count++]];

  const uint32_t n = (4 + (c.id << 2));
  InitEnemyDataSTD(e, c.x, c.y, n);
}

// --- Main execution loop ---
bool SclVM::Execute() {
  bool bFlag = true;

  while (bFlag) {
    const auto *cmd = m_pc;
    switch (cmd[0]) {
    case SCL_KEY:
      if ((Key_Data & (KEY_TAMA | KEY_RETURN | KEY_BOMB)) ||
          ++SclKeyWaitCount >= 180) {
        m_pc += SclCmdLength(cmd[0]);
        SclKeyWaitCount = 0;
      } else {
        bFlag = false;
      }
      break;

    case SCL_TIME: {
      auto c = Decode(ScmdTag<SCL_TIME>{}, cmd);
      if (SclInfo.MsgFlag) {
        if ((Key_Data & KEY_TAMA) || (Key_Data & KEY_RETURN) ||
            (Key_Data & KEY_BOMB)) {
          if (!SclInfo.ReturnFlag) {
            GameCount = c.time;
            SclInfo.ReturnFlag = true;
          }
        } else {
          SclInfo.ReturnFlag = false;
        }
      }
      if (c.time > GameCount) {
        bFlag = false;
      } else {
        m_pc += SclCmdLength(SCL_TIME);
      }
    } break;

    case SCL_ENEMY: {
      auto c = Decode(ScmdTag<SCL_ENEMY>{}, cmd);
      if (BossNow == 0)
        SpawnEnemy(c);
      m_pc += SclCmdLength(SCL_ENEMY);
    } break;

    case SCL_BOSS: {
      auto c = Decode(ScmdTag<SCL_BOSS>{}, cmd);
      BossSet(c.x, c.y, c.id);
      m_pc += SclCmdLength(SCL_BOSS);
    } break;

    case SCL_BOSSDEAD:
      BossKillAll();
      m_pc += SclCmdLength(cmd[0]);
      break;

    case SCL_MWOPEN:
      if (!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE))
        MWinOpen();
      SclInfo.MsgFlag = true;
      m_pc += SclCmdLength(cmd[0]);
      break;

    case SCL_MWCLOSE:
      if (!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE))
        MWinClose();
      SclInfo.MsgFlag = false;
      m_pc += SclCmdLength(cmd[0]);
      break;

    case SCL_MSG:
      MWinMsg(reinterpret_cast<const char *>(cmd + 1));
      m_pc += (strlen(reinterpret_cast<const char *>(cmd + 1)) + 2);
      break;

    case SCL_FACE: {
      auto c = Decode(ScmdTag<SCL_FACE>{}, cmd);
      MWinFace(c.face_id);
      m_pc += SclCmdLength(SCL_FACE);
    } break;

    case SCL_LOADFACE: {
      auto c = Decode(ScmdTag<SCL_LOADFACE>{}, cmd);
      LoadFace(c.surf_id, c.file_no);
      m_pc += SclCmdLength(SCL_LOADFACE);
    } break;

    case SCL_NPG:
      MWinCmd(MWCMD_NEWPAGE);
      m_pc += SclCmdLength(cmd[0]);
      break;

    case SCL_END:
      return true;

    case SCL_SSP: {
      auto c = Decode(ScmdTag<SCL_SSP>{}, cmd);
      ScrollSpeed(c.speed);
      m_pc += SclCmdLength(SCL_SSP);
    } break;

    case SCL_MUSIC: {
      auto c = Decode(ScmdTag<SCL_MUSIC>{}, cmd);
      if (!IsDemoplay) {
        BGM_Stop();
        if (BGM_Switch(c.track)) {
          BGM_Play();
          const auto mtitle = BGM_Title();
          if (!mtitle.empty())
            SetMusicTitle(460, mtitle);
        }
      }
      m_pc += SclCmdLength(SCL_MUSIC);
    } break;

    case SCL_DELENEMY:
      enemyind_set();
      m_pc += SclCmdLength(cmd[0]);
      break;

    case SCL_EFC: {
      auto c = Decode(ScmdTag<SCL_EFC>{}, cmd);
      switch (c.efc_id) {
      case SEFC_WARN:
        Snd_SEPlay(8, GX_MID, true);
        WarningEffectSet();
        break;
      case SEFC_WARNSTOP:
        Snd_SEStop(8);
        break;
      case SEFC_MUSICFADE:
        BGM_FadeOut(120);
        break;
      case SEFC_STG2BOSS:
        ScrollCommand(SCMD_STG2BOSS);
        break;
      case SEFC_RASTERON:
        ScrollCommand(SCMD_RASTER_ON);
        break;
      case SEFC_RASTEROFF:
        ScrollCommand(SCMD_RASTER_OFF);
        break;
      case SEFC_STG3BOSS:
        ScrollCommand(SCMD_STG3BOSS);
        break;
      case SEFC_STG3RESET:
        ScrollCommand(SCMD_STG3RESET);
        break;
      case SEFC_CFADEIN:
        ScreenEffectSet(SCNEFC_CFADEIN);
        break;
      case SEFC_CFADEOUT:
        ScreenEffectSet(SCNEFC_CFADEOUT);
        break;
      case SEFC_STG6CUBE:
        ScrollCommand(SCMD_STG6CUBE);
        break;
      case SEFC_STG6RNDECL:
        ScrollCommand(SCMD_STG6RNDECL);
        break;
      case SEFC_STG4ROCK:
        ScrollCommand(SCMD_STG4ROCK);
        break;
      case SEFC_STG4LEAVE:
        ScrollCommand(SCMD_STG4LEAVE);
        break;
      case SEFC_WHITEIN:
        ScreenEffectSet(SCNEFC_WHITEIN);
        break;
      case SEFC_WHITEOUT:
        ScreenEffectSet(SCNEFC_WHITEOUT);
        break;
      case SEFC_LOADEX01:
        LoadGraph(GRAPH_ID_EXBOSS1);
        break;
      case SEFC_LOADEX02:
        LoadGraph(GRAPH_ID_EXBOSS2);
        break;
      case SEFC_STG6RASTER:
        ScrollCommand(SCMD_STG6RASTER);
        break;
      }
      m_pc += SclCmdLength(SCL_EFC);
    } break;

    case SCL_WAITEX: {
      auto c = Decode(ScmdTag<SCL_WAITEX>{}, cmd);
      switch (c.cond) {
      case SWAIT_BOSSHP:
        if (GetBossHPSum() <= c.value)
          break;
        return false;
      case SWAIT_BOSSLEFT:
        if (BossNow <= c.value)
          break;
        return false;
      }
      m_pc += SclCmdLength(SCL_WAITEX);
    } break;

    case SCL_STAGECLEAR:
      if (DemoplaySaveAllEnable) {
        DemoplayFlushStage();
        GameNextStage();
        return true;
      }
      if (DemoplayLoadAllEnable) {
        if (GameStage < PlaybackMaxStage)
          GameNextStage();
        return true;
      }
      GameNextStage();
      return true;

    case SCL_GAMECLEAR:
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
        case 0:
          ConfigDat.ExtraStgFlags.v |= 1;
          break;
        case 1:
          ConfigDat.ExtraStgFlags.v |= 2;
          break;
        case 2:
          ConfigDat.ExtraStgFlags.v |= 4;
          break;
        }
      }
      ConfigSave();
      EndingInit();
      return true;

    case SCL_EXTRACLEAR:
      if (DemoplaySaveAllEnable) {
        DemoplayFlushStage();
        DemoplaySaveReplayAll(true);
      }
      if (DemoplayLoadAllEnable)
        return true;
      NameRegistInit(true);
      return true;

    case SCL_MAPPALETTE:
      GrpSurface_PaletteApplyToBackend(SURFACE_ID::MAPCHIP);
      m_pc += SclCmdLength(cmd[0]);
      break;

    case SCL_ENEMYPALETTE:
      LoadPaletteFromEnemy();
      m_pc += SclCmdLength(cmd[0]);
      break;

    default:
      MWinOpen();
      MWinCmd(MWCMD_NEWPAGE);
      MWinCmd(MWCMD_LARGEFONT);
      MWinMsg("バグ発生だにょ");
      return false;
    }
  }

  // Advance stage timer and update play rank
  GameCount++;
  if ((GameCount & 0x3f) == 0) {
    if (GameStage == GRAPH_ID_EXSTAGE)
      PlayRankAdd(1);
    else
      PlayRankAdd(1 + GameStage / 3);
  }

  return false;
}
