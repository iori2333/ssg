/*                                                                           */
/*   DemoPlay.cpp   デモプレイ処理                                           */
/*                                                                           */
/*                                                                           */

#include "CONFIG.h"
#include "DEMOPLAY.h"
#include "demo_manager.h"
#include "GIAN.h"
#include "LZ_UTY.h"
#include "game/input.h"
#include "game/ut_math.h"
#include "platform/file.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <chrono>
#include <ctime>

// DemoplayLoadEnable, DemoplaySaveAllEnable, DemoplayLoadAllEnable,
// Demos.demo_info, Demos.demo_buffer[] → demo_manager.cpp の DemoManager に移動
static uint32_t DemoFrameCur;
static struct ConfigTempData {
  uint8_t PlayerStock;
  uint8_t BombStock;
  uint8_t InputFlags;
} ConfigTemp; // コンフィグのデータ一時保存用

// Multi-stage recording state
static std::vector<std::vector<INPUT_BITS>> StageRecordBufs;
static uint8_t MultiStageCount = 0;
static uint8_t MultiStageNums[REPLAY_STAGE_MAX] = {};
static uint32_t MultiStageFrames[REPLAY_STAGE_MAX] = {};

// Multi-stage playback state
static std::vector<INPUT_BITS> AllPlaybackBuf;
// Demos.multi_play_info, Demos.playback_max_stage, PendingReplayFile → demo_manager.cpp に移動

std::u8string ReplayAllFN(bool exstg) {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  struct tm tm;
  localtime_s(&tm, &time);
  char buf[64];
  if (exstg) {
    sprintf_s(buf, "replay_ex_%04d%02d%02d_%02d%02d%02d.DAT",
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
              tm.tm_hour, tm.tm_min, tm.tm_sec);
  } else {
    sprintf_s(buf, "replay_%04d%02d%02d_%02d%02d%02d.DAT",
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
              tm.tm_hour, tm.tm_min, tm.tm_sec);
  }
  return std::u8string(reinterpret_cast<const char8_t *>(buf));
}

void DemoplayInit(void) {
  // 乱数の準備 //
  Demos.demo_info.RndSeed =
      ((Cast::up<uint32_t>(rnd()) + 1u) * (Cast::up<uint32_t>(rnd()) + 1u));
  rnd_seed_set(Demos.demo_info.RndSeed);

  Demos.demo_info.Exp = Viv.exp;
  Demos.demo_info.Weapon = Viv.weapon;
  Demos.demo_info.CfgDat.GameLevel = GameLevel;
  Demos.demo_info.CfgDat.PlayerStock = Viv.left;
  Demos.demo_info.CfgDat.BombStock = ConfigDat.BombStock.v;
  Demos.demo_info.CfgDat.InputFlags = ConfigDat.InputFlags.v;

  DemoFrameCur = 0;
  DemoplaySaveAllEnable = true;
  MultiStageCount = 0;
  StageRecordBufs.clear();
}

bool DemoplayHasRecordedStages(void) {
  return (DemoplaySaveAllEnable && (MultiStageCount > 0 || DemoFrameCur > 0));
}

void DemoplayFlushStage(void) {
  if (!DemoplaySaveAllEnable)
    return;
  if (DemoFrameCur == 0)
    return;

  if (MultiStageCount < REPLAY_STAGE_MAX) {
    MultiStageNums[MultiStageCount] = GameStage;
    MultiStageFrames[MultiStageCount] = DemoFrameCur;

    std::vector<INPUT_BITS> stage_data(Demos.demo_buffer.data(), Demos.demo_buffer.data() + DemoFrameCur);
    StageRecordBufs.push_back(std::move(stage_data));

    MultiStageCount++;
  }

  DemoFrameCur = 0;
}

bool DemoplayLoadSetup() {
  DemoFrameCur = 0;
  DemoplayLoadEnable = true;

  // コンフィグの初期化 //
  // 現在のコンフィグを保持する //
  ConfigTemp.PlayerStock = ConfigDat.PlayerStock.v;
  ConfigTemp.BombStock = ConfigDat.BombStock.v;
  ConfigTemp.InputFlags = ConfigDat.InputFlags.v;

  // そのときのコンフィグを転送 //
  ConfigDat.BombStock.v = Demos.demo_info.CfgDat.BombStock;
  ConfigDat.PlayerStock.v = Demos.demo_info.CfgDat.PlayerStock;
  ConfigDat.InputFlags.v = Demos.demo_info.CfgDat.InputFlags;
  GameLevel = Demos.demo_info.CfgDat.GameLevel;

  // 本体の性能記述 //
  Viv.exp = Demos.demo_info.Exp;
  Viv.weapon = Demos.demo_info.Weapon;
  Viv.left = ConfigDat.PlayerStock.v;
  Viv.bomb = ConfigDat.BombStock.v;

  // 乱数の初期化 //
  // 最後に乱数もそろえる //
  rnd_seed_set(Demos.demo_info.RndSeed);

  return true;
}

bool DemoplayRecord(INPUT_BITS key) {
  if (!DemoplaySaveAllEnable) {
    return false;
  }

  Demos.demo_buffer[DemoFrameCur++] = key;

  // バッファが最後に来たか、ＥＳＣが押された場合 //
  if ((DemoFrameCur == DEMOBUF_MAX) || (key & KEY_ESC)) {
    DemoFrameCur--;
    return true;
  }
  return false;
}

void DemoplaySaveDemo(void) {
  if (!DemoplaySaveAllEnable)
    return;

  Demos.demo_buffer[DemoFrameCur] = KEY_ESC;
  Demos.demo_info.FrameCount = (DemoFrameCur + 1);

  char8_t fn[] = u8"STG_Demo.DAT";
  fn[3] = ('0' + GameStage);

  auto *f = SDL_IOFromFile(fn, "wb");
  if (f) {
    SDL_WriteIO(f, &Demos.demo_info, sizeof(Demos.demo_info));
    SDL_WriteIO(f, Demos.demo_buffer.data(), (sizeof(Demos.demo_buffer[0]) * Demos.demo_info.FrameCount));
    SDL_CloseIO(f);
  }
}

bool DemoplayLoadDemo(int stage) {
  // 展開 //
  const auto temp = LoadDemo(stage);
  auto temp_cursor = temp.cursor();
  {
    const auto maybe_info = temp_cursor.next<DEMOPLAY_INFO>();
    if (!maybe_info) {
      return false;
    }
    Demos.demo_info = maybe_info.value()[0];
  }
  {
    const auto maybe_inputs = temp_cursor.next<uint16_t>(Demos.demo_info.FrameCount);
    if (!maybe_inputs) {
      return false;
    }
    const auto inputs = maybe_inputs.value();
    memcpy(Demos.demo_buffer.data(), inputs.data(), inputs.size_bytes());
  }
  return DemoplayLoadSetup();
}

INPUT_BITS DemoplayMove(void) {
  if (!DemoplayLoadEnable)
    return KEY_ESC;

  const auto ptr = DemoFrameCur;
  if (ptr >= Demos.demo_info.FrameCount) {
    DemoplayLoadEnable = false;
    return KEY_ESC;
  }

  DemoFrameCur++;
  return Demos.demo_buffer[ptr];
}

void DemoplayCleanup(void) {
  ConfigDat.PlayerStock.v = ConfigTemp.PlayerStock;
  ConfigDat.BombStock.v = ConfigTemp.BombStock;
  ConfigDat.InputFlags.v = ConfigTemp.InputFlags;

  DemoplayLoadEnable = false;
  DemoplayLoadAllEnable = false;
}

void DemoplaySaveReplayAll(bool exstg) {
  if (!DemoplaySaveAllEnable)
    return;

  // Flush current stage data if any (not yet flushed by stage clear)
  if (DemoFrameCur > 0 && MultiStageCount < REPLAY_STAGE_MAX) {
    MultiStageNums[MultiStageCount] = GameStage;
    MultiStageFrames[MultiStageCount] = DemoFrameCur;
    StageRecordBufs.emplace_back(Demos.demo_buffer.data(), Demos.demo_buffer.data() + DemoFrameCur);
    MultiStageCount++;
    DemoFrameCur = 0;
  }

  DemoplaySaveAllEnable = false;

  MULTI_REPLAY_INFO info = {};
  info.RndSeed = Demos.demo_info.RndSeed;
  info.StageCount = MultiStageCount;
  info.CfgDat = Demos.demo_info.CfgDat;
  info.Exp = Demos.demo_info.Exp;
  info.Weapon = Demos.demo_info.Weapon;
  for (int i = 0; i < MultiStageCount; i++) {
    info.Stages[i] = MultiStageNums[i];
    info.FrameCounts[i] = MultiStageFrames[i];
  }

  PACKFILE_WRITE out;
  out.files.push_back(
      {reinterpret_cast<const uint8_t *>(&info), sizeof(info)});
  for (auto &buf : StageRecordBufs) {
    out.files.push_back(
        {reinterpret_cast<const uint8_t *>(buf.data()),
         buf.size() * sizeof(INPUT_BITS)});
  }

  const auto fn = ReplayAllFN(exstg);
  out.Write(fn.c_str());

  StageRecordBufs.clear();
  MultiStageCount = 0;
}

bool DemoplayLoadReplayAll(const char8_t *fn) {
  const auto in = FilStartR(fn);
  if (!in)
    return false;

  BYTE_BUFFER_OWNED temp = in.MemExpand(0);
  if (nullptr == temp)
    return false;
  memcpy(&Demos.multi_play_info, temp.get(), sizeof(MULTI_REPLAY_INFO));

  // Compute max stage for stage transition gating
  Demos.playback_max_stage = 0;
  for (uint8_t i = 0; i < Demos.multi_play_info.StageCount; i++) {
    if (Demos.multi_play_info.Stages[i] > Demos.playback_max_stage)
      Demos.playback_max_stage = Demos.multi_play_info.Stages[i];
  }

  AllPlaybackBuf.clear();
  uint32_t total_frames = 0;
  for (uint8_t i = 0; i < Demos.multi_play_info.StageCount; i++) {
    temp = in.MemExpand(i + 1);
    if (nullptr == temp)
      return false;
    uint32_t n_frames = Demos.multi_play_info.FrameCounts[i];
    const auto src = reinterpret_cast<const INPUT_BITS *>(temp.get());
    AllPlaybackBuf.insert(AllPlaybackBuf.end(), src, src + n_frames);
    total_frames += n_frames;
  }

  // Copy combined data into Demos.demo_buffer for DemoplayMove()
  if (total_frames > DEMOBUF_MAX)
    total_frames = DEMOBUF_MAX;
  memcpy(Demos.demo_buffer.data(), AllPlaybackBuf.data(),
         total_frames * sizeof(INPUT_BITS));
  Demos.demo_info.FrameCount = total_frames;
  Demos.demo_info.RndSeed = Demos.multi_play_info.RndSeed;
  Demos.demo_info.CfgDat = Demos.multi_play_info.CfgDat;
  Demos.demo_info.Exp = Demos.multi_play_info.Exp;
  Demos.demo_info.Weapon = Demos.multi_play_info.Weapon;

  DemoplayLoadAllEnable = true;
  return DemoplayLoadSetup();
}
