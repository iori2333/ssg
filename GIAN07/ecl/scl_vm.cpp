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

// --- Spawn a single enemy from Scmd::ENEMY data ---
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
    switch (static_cast<Scmd>(cmd[0])) {
    case Scmd::KEY:
      if ((Key_Data & (KEY_TAMA | KEY_RETURN | KEY_BOMB)) ||
          ++SclKeyWaitCount >= 180) {
        m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
        SclKeyWaitCount = 0;
      } else {
        bFlag = false;
      }
      break;

    case Scmd::TIME: {
      auto c = Decode(ScmdTag<Scmd::TIME>{}, cmd);
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
        m_pc += SclCmdLength(Scmd::TIME);
      }
    } break;

    case Scmd::ENEMY: {
      auto c = Decode(ScmdTag<Scmd::ENEMY>{}, cmd);
      if (BossNow == 0)
        SpawnEnemy(c);
      m_pc += SclCmdLength(Scmd::ENEMY);
    } break;

    case Scmd::BOSS: {
      auto c = Decode(ScmdTag<Scmd::BOSS>{}, cmd);
      BossSet(c.x, c.y, c.id);
      m_pc += SclCmdLength(Scmd::BOSS);
    } break;

    case Scmd::BOSSDEAD:
      BossKillAll();
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
      break;

    case Scmd::MWOPEN:
      if (!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE))
        MWinOpen();
      SclInfo.MsgFlag = true;
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
      break;

    case Scmd::MWCLOSE:
      if (!(ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE))
        MWinClose();
      SclInfo.MsgFlag = false;
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
      break;

    case Scmd::MSG:
      MWinMsg(reinterpret_cast<const char *>(cmd + 1));
      m_pc += (strlen(reinterpret_cast<const char *>(cmd + 1)) + 2);
      break;

    case Scmd::FACE: {
      auto c = Decode(ScmdTag<Scmd::FACE>{}, cmd);
      MWinFace(c.face_id);
      m_pc += SclCmdLength(Scmd::FACE);
    } break;

    case Scmd::LOADFACE: {
      auto c = Decode(ScmdTag<Scmd::LOADFACE>{}, cmd);
      LoadFace(c.surf_id, c.file_no);
      m_pc += SclCmdLength(Scmd::LOADFACE);
    } break;

    case Scmd::NPG:
      MWinCmd(MWCMD_NEWPAGE);
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
      break;

    case Scmd::END:
      return true;

    case Scmd::SSP: {
      auto c = Decode(ScmdTag<Scmd::SSP>{}, cmd);
      ScrollSpeed(c.speed);
      m_pc += SclCmdLength(Scmd::SSP);
    } break;

    case Scmd::MUSIC: {
      auto c = Decode(ScmdTag<Scmd::MUSIC>{}, cmd);
      if (!IsDemoplay) {
        BGM_Stop();
        if (BGM_Switch(c.track)) {
          BGM_Play();
          const auto mtitle = BGM_Title();
          if (!mtitle.empty())
            SetMusicTitle(460, mtitle);
        }
      }
      m_pc += SclCmdLength(Scmd::MUSIC);
    } break;

    case Scmd::DELENEMY:
      enemyind_set();
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
      break;

    case Scmd::EFC: {
      auto c = Decode(ScmdTag<Scmd::EFC>{}, cmd);
      switch (static_cast<Sefc>(c.efc_id)) {
      case Sefc::WARN:
        Snd_SEPlay(8, GX_MID, true);
        WarningEffectSet();
        break;
      case Sefc::WARNSTOP:
        Snd_SEStop(8);
        break;
      case Sefc::MUSICFADE:
        BGM_FadeOut(120);
        break;
      case Sefc::STG2BOSS:
        ScrollCommand(SCMD_STG2BOSS);
        break;
      case Sefc::RASTERON:
        ScrollCommand(SCMD_RASTER_ON);
        break;
      case Sefc::RASTEROFF:
        ScrollCommand(SCMD_RASTER_OFF);
        break;
      case Sefc::STG3BOSS:
        ScrollCommand(SCMD_STG3BOSS);
        break;
      case Sefc::STG3RESET:
        ScrollCommand(SCMD_STG3RESET);
        break;
      case Sefc::CFADEIN:
        ScreenEffectSet(SCNEFC_CFADEIN);
        break;
      case Sefc::CFADEOUT:
        ScreenEffectSet(SCNEFC_CFADEOUT);
        break;
      case Sefc::STG6CUBE:
        ScrollCommand(SCMD_STG6CUBE);
        break;
      case Sefc::STG6RNDECL:
        ScrollCommand(SCMD_STG6RNDECL);
        break;
      case Sefc::STG4ROCK:
        ScrollCommand(SCMD_STG4ROCK);
        break;
      case Sefc::STG4LEAVE:
        ScrollCommand(SCMD_STG4LEAVE);
        break;
      case Sefc::WHITEIN:
        ScreenEffectSet(SCNEFC_WHITEIN);
        break;
      case Sefc::WHITEOUT:
        ScreenEffectSet(SCNEFC_WHITEOUT);
        break;
      case Sefc::LOADEX01:
        LoadGraph(GRAPH_ID_EXBOSS1);
        break;
      case Sefc::LOADEX02:
        LoadGraph(GRAPH_ID_EXBOSS2);
        break;
      case Sefc::STG6RASTER:
        ScrollCommand(SCMD_STG6RASTER);
        break;
      }
      m_pc += SclCmdLength(Scmd::EFC);
    } break;

    case Scmd::WAITEX: {
      auto c = Decode(ScmdTag<Scmd::WAITEX>{}, cmd);
      switch (static_cast<Swait>(c.cond)) {
      case Swait::BOSSHP:
        if (GetBossHPSum() <= c.value)
          break;
        return false;
      case Swait::BOSSLEFT:
        if (BossNow <= c.value)
          break;
        return false;
      }
      m_pc += SclCmdLength(Scmd::WAITEX);
    } break;

    case Scmd::STAGECLEAR:
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

    case Scmd::GAMECLEAR:
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

    case Scmd::EXTRACLEAR:
      if (DemoplaySaveAllEnable) {
        DemoplayFlushStage();
        DemoplaySaveReplayAll(true);
      }
      if (DemoplayLoadAllEnable)
        return true;
      NameRegistInit(true);
      return true;

    case Scmd::MAPPALETTE:
      GrpSurface_PaletteApplyToBackend(SURFACE_ID::MAPCHIP);
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
      break;

    case Scmd::ENEMYPALETTE:
      LoadPaletteFromEnemy();
      m_pc += SclCmdLength(static_cast<Scmd>(cmd[0]));
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
