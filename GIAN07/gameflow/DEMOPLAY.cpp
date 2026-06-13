/*                                                                           */
/*   DemoPlay.cpp   デモプレイ処理                                           */
/*                                                                           */
/*                                                                           */

#include "CONFIG.h"
#include "DEMOPLAY.h"
#include "GIAN.h"
#include "LZ_UTY.h"
#include "game/input.h"
#include "game/ut_math.h"
#include "platform/file.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <chrono>
#include <ctime>

bool DemoplayLoadEnable = false;    // デモプレイのロードが動作しているか
bool DemoplaySaveAllEnable = false; // Multi-stage recording active
bool DemoplayLoadAllEnable = false; // Multi-stage playback active
DEMOPLAY_INFO DemoInfo;             // デモプレイ情報
INPUT_BITS DemoBuffer[DEMOBUF_MAX]; // デモプレイ用バッファ
static uint32_t DemoFrameCur;
struct {
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
MULTI_REPLAY_INFO MultiPlayInfo;
uint8_t PlaybackMaxStage = 0;
std::u8string PendingReplayFile;

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
  DemoInfo.RndSeed =
      ((Cast::up<uint32_t>(rnd()) + 1u) * (Cast::up<uint32_t>(rnd()) + 1u));
  rnd_seed_set(DemoInfo.RndSeed);

  DemoInfo.Exp = Viv.exp;
  DemoInfo.Weapon = Viv.weapon;
  DemoInfo.CfgDat.GameLevel = GameLevel;
  DemoInfo.CfgDat.PlayerStock = Viv.left;
  DemoInfo.CfgDat.BombStock = ConfigDat.BombStock.v;
  DemoInfo.CfgDat.InputFlags = ConfigDat.InputFlags.v;

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

    std::vector<INPUT_BITS> stage_data(DemoBuffer, DemoBuffer + DemoFrameCur);
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
  ConfigDat.BombStock.v = DemoInfo.CfgDat.BombStock;
  ConfigDat.PlayerStock.v = DemoInfo.CfgDat.PlayerStock;
  ConfigDat.InputFlags.v = DemoInfo.CfgDat.InputFlags;
  GameLevel = DemoInfo.CfgDat.GameLevel;

  // 本体の性能記述 //
  Viv.exp = DemoInfo.Exp;
  Viv.weapon = DemoInfo.Weapon;
  Viv.left = ConfigDat.PlayerStock.v;
  Viv.bomb = ConfigDat.BombStock.v;

  // 乱数の初期化 //
  // 最後に乱数もそろえる //
  rnd_seed_set(DemoInfo.RndSeed);

  return true;
}

bool DemoplayRecord(INPUT_BITS key) {
  if (!DemoplaySaveAllEnable) {
    return false;
  }

  DemoBuffer[DemoFrameCur++] = key;

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

  DemoBuffer[DemoFrameCur] = KEY_ESC;
  DemoInfo.FrameCount = (DemoFrameCur + 1);

  char8_t fn[] = u8"STG_Demo.DAT";
  fn[3] = ('0' + GameStage);

  auto *f = SDL_IOFromFile(fn, "wb");
  if (f) {
    SDL_WriteIO(f, &DemoInfo, sizeof(DemoInfo));
    SDL_WriteIO(f, DemoBuffer, (sizeof(DemoBuffer[0]) * DemoInfo.FrameCount));
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
    DemoInfo = maybe_info.value()[0];
  }
  {
    const auto maybe_inputs = temp_cursor.next<uint16_t>(DemoInfo.FrameCount);
    if (!maybe_inputs) {
      return false;
    }
    const auto inputs = maybe_inputs.value();
    memcpy(DemoBuffer, inputs.data(), inputs.size_bytes());
  }
  return DemoplayLoadSetup();
}

INPUT_BITS DemoplayMove(void) {
  if (!DemoplayLoadEnable)
    return KEY_ESC;

  const auto ptr = DemoFrameCur;
  if (ptr >= DemoInfo.FrameCount) {
    DemoplayLoadEnable = false;
    return KEY_ESC;
  }

  DemoFrameCur++;
  return DemoBuffer[ptr];
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
    StageRecordBufs.emplace_back(DemoBuffer, DemoBuffer + DemoFrameCur);
    MultiStageCount++;
    DemoFrameCur = 0;
  }

  DemoplaySaveAllEnable = false;

  MULTI_REPLAY_INFO info = {};
  info.RndSeed = DemoInfo.RndSeed;
  info.StageCount = MultiStageCount;
  info.CfgDat = DemoInfo.CfgDat;
  info.Exp = DemoInfo.Exp;
  info.Weapon = DemoInfo.Weapon;
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
  memcpy(&MultiPlayInfo, temp.get(), sizeof(MULTI_REPLAY_INFO));

  // Compute max stage for stage transition gating
  PlaybackMaxStage = 0;
  for (uint8_t i = 0; i < MultiPlayInfo.StageCount; i++) {
    if (MultiPlayInfo.Stages[i] > PlaybackMaxStage)
      PlaybackMaxStage = MultiPlayInfo.Stages[i];
  }

  AllPlaybackBuf.clear();
  uint32_t total_frames = 0;
  for (uint8_t i = 0; i < MultiPlayInfo.StageCount; i++) {
    temp = in.MemExpand(i + 1);
    if (nullptr == temp)
      return false;
    uint32_t n_frames = MultiPlayInfo.FrameCounts[i];
    const auto src = reinterpret_cast<const INPUT_BITS *>(temp.get());
    AllPlaybackBuf.insert(AllPlaybackBuf.end(), src, src + n_frames);
    total_frames += n_frames;
  }

  // Copy combined data into DemoBuffer for DemoplayMove()
  if (total_frames > DEMOBUF_MAX)
    total_frames = DEMOBUF_MAX;
  memcpy(DemoBuffer, AllPlaybackBuf.data(),
         total_frames * sizeof(INPUT_BITS));
  DemoInfo.FrameCount = total_frames;
  DemoInfo.RndSeed = MultiPlayInfo.RndSeed;
  DemoInfo.CfgDat = MultiPlayInfo.CfgDat;
  DemoInfo.Exp = MultiPlayInfo.Exp;
  DemoInfo.Weapon = MultiPlayInfo.Weapon;

  DemoplayLoadAllEnable = true;
  return DemoplayLoadSetup();
}
