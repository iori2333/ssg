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

// ファイル静的変数 → demo_manager.h の DemoManager struct に移動

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

void DemoManager::Init(void) {
  // 乱数の準備 //
  this->demo_info.RndSeed =
      ((Cast::up<uint32_t>(rnd()) + 1u) * (Cast::up<uint32_t>(rnd()) + 1u));
  rnd_seed_set(this->demo_info.RndSeed);

  this->demo_info.Exp = Viv.exp;
  this->demo_info.Weapon = Viv.weapon;
  this->demo_info.CfgDat.GameLevel = GameLevel;
  this->demo_info.CfgDat.PlayerStock = Viv.left;
  this->demo_info.CfgDat.BombStock = ConfigDat.BombStock.v;
  this->demo_info.CfgDat.InputFlags = ConfigDat.InputFlags.v;

  this->demo_frame_cur = 0;
  this->save_all_enable = true;
  this->multi_stage_count = 0;
  this->stage_record_bufs.clear();
}

bool DemoManager::HasRecordedStages(void) {
  return (this->save_all_enable && (this->multi_stage_count > 0 || this->demo_frame_cur > 0));
}

void DemoManager::FlushStage(void) {
  if (!this->save_all_enable)
    return;
  if (this->demo_frame_cur == 0)
    return;

  if (this->multi_stage_count < REPLAY_STAGE_MAX) {
    this->multi_stage_nums[this->multi_stage_count] = GameStage;
    this->multi_stage_frames[this->multi_stage_count] = this->demo_frame_cur;

    std::vector<INPUT_BITS> stage_data(this->demo_buffer.data(), this->demo_buffer.data() + this->demo_frame_cur);
    this->stage_record_bufs.push_back(std::move(stage_data));

    this->multi_stage_count++;
  }

  this->demo_frame_cur = 0;
}

bool DemoManager::LoadSetup() {
  this->demo_frame_cur = 0;
  this->load_enable = true;

  // コンフィグの初期化 //
  // 現在のコンフィグを保持する //
  this->config_temp.PlayerStock = ConfigDat.PlayerStock.v;
  this->config_temp.BombStock = ConfigDat.BombStock.v;
  this->config_temp.InputFlags = ConfigDat.InputFlags.v;

  // そのときのコンフィグを転送 //
  ConfigDat.BombStock.v = this->demo_info.CfgDat.BombStock;
  ConfigDat.PlayerStock.v = this->demo_info.CfgDat.PlayerStock;
  ConfigDat.InputFlags.v = this->demo_info.CfgDat.InputFlags;
  GameLevel = this->demo_info.CfgDat.GameLevel;

  // 本体の性能記述 //
  Viv.exp = this->demo_info.Exp;
  Viv.weapon = this->demo_info.Weapon;
  Viv.left = ConfigDat.PlayerStock.v;
  Viv.bomb = ConfigDat.BombStock.v;

  // 乱数の初期化 //
  // 最後に乱数もそろえる //
  rnd_seed_set(this->demo_info.RndSeed);

  return true;
}

bool DemoManager::Record(INPUT_BITS key) {
  if (!this->save_all_enable) {
    return false;
  }

  this->demo_buffer[this->demo_frame_cur++] = key;

  // バッファが最後に来たか、ＥＳＣが押された場合 //
  if ((this->demo_frame_cur == DEMOBUF_MAX) || (key & KEY_ESC)) {
    this->demo_frame_cur--;
    return true;
  }
  return false;
}

void DemoManager::SaveDemo(void) {
  if (!this->save_all_enable)
    return;

  this->demo_buffer[this->demo_frame_cur] = KEY_ESC;
  this->demo_info.FrameCount = (this->demo_frame_cur + 1);

  char8_t fn[] = u8"STG_Demo.DAT";
  fn[3] = ('0' + GameStage);

  auto *f = SDL_IOFromFile(fn, "wb");
  if (f) {
    SDL_WriteIO(f, &this->demo_info, sizeof(this->demo_info));
    SDL_WriteIO(f, this->demo_buffer.data(), (sizeof(this->demo_buffer[0]) * this->demo_info.FrameCount));
    SDL_CloseIO(f);
  }
}

bool DemoManager::LoadDemo(int stage) {
  // 展開 //
  const auto temp = ::LoadDemo(stage);
  auto temp_cursor = temp.cursor();
  {
    const auto maybe_info = temp_cursor.next<DEMOPLAY_INFO>();
    if (!maybe_info) {
      return false;
    }
    this->demo_info = maybe_info.value()[0];
  }
  {
    const auto maybe_inputs = temp_cursor.next<uint16_t>(this->demo_info.FrameCount);
    if (!maybe_inputs) {
      return false;
    }
    const auto inputs = maybe_inputs.value();
    memcpy(this->demo_buffer.data(), inputs.data(), inputs.size_bytes());
  }
  return this->LoadSetup();
}

INPUT_BITS DemoManager::Move(void) {
  if (!this->load_enable)
    return KEY_ESC;

  const auto ptr = this->demo_frame_cur;
  if (ptr >= this->demo_info.FrameCount) {
    this->load_enable = false;
    return KEY_ESC;
  }

  this->demo_frame_cur++;
  return this->demo_buffer[ptr];
}

void DemoManager::Cleanup(void) {
  ConfigDat.PlayerStock.v = this->config_temp.PlayerStock;
  ConfigDat.BombStock.v = this->config_temp.BombStock;
  ConfigDat.InputFlags.v = this->config_temp.InputFlags;

  this->load_enable = false;
  this->load_all_enable = false;
}

void DemoManager::SaveReplayAll(bool exstg) {
  if (!this->save_all_enable)
    return;

  // Flush current stage data if any (not yet flushed by stage clear)
  if (this->demo_frame_cur > 0 && this->multi_stage_count < REPLAY_STAGE_MAX) {
    this->multi_stage_nums[this->multi_stage_count] = GameStage;
    this->multi_stage_frames[this->multi_stage_count] = this->demo_frame_cur;
    this->stage_record_bufs.emplace_back(this->demo_buffer.data(), this->demo_buffer.data() + this->demo_frame_cur);
    this->multi_stage_count++;
    this->demo_frame_cur = 0;
  }

  this->save_all_enable = false;

  MULTI_REPLAY_INFO info = {};
  info.RndSeed = this->demo_info.RndSeed;
  info.StageCount = this->multi_stage_count;
  info.CfgDat = this->demo_info.CfgDat;
  info.Exp = this->demo_info.Exp;
  info.Weapon = this->demo_info.Weapon;
  for (int i = 0; i < this->multi_stage_count; i++) {
    info.Stages[i] = this->multi_stage_nums[i];
    info.FrameCounts[i] = this->multi_stage_frames[i];
  }

  PACKFILE_WRITE out;
  out.files.push_back(
      {reinterpret_cast<const uint8_t *>(&info), sizeof(info)});
  for (auto &buf : this->stage_record_bufs) {
    out.files.push_back(
        {reinterpret_cast<const uint8_t *>(buf.data()),
         buf.size() * sizeof(INPUT_BITS)});
  }

  const auto fn = ReplayAllFN(exstg);
  out.Write(fn.c_str());

  this->stage_record_bufs.clear();
  this->multi_stage_count = 0;
}

bool DemoManager::LoadReplayAll(const char8_t *fn) {
  const auto in = FilStartR(fn);
  if (!in)
    return false;

  BYTE_BUFFER_OWNED temp = in.MemExpand(0);
  if (nullptr == temp)
    return false;
  memcpy(&this->multi_play_info, temp.get(), sizeof(MULTI_REPLAY_INFO));

  // Compute max stage for stage transition gating
  this->playback_max_stage = 0;
  for (uint8_t i = 0; i < this->multi_play_info.StageCount; i++) {
    if (this->multi_play_info.Stages[i] > this->playback_max_stage)
      this->playback_max_stage = this->multi_play_info.Stages[i];
  }

  this->all_playback_buf.clear();
  uint32_t total_frames = 0;
  for (uint8_t i = 0; i < this->multi_play_info.StageCount; i++) {
    temp = in.MemExpand(i + 1);
    if (nullptr == temp)
      return false;
    uint32_t n_frames = this->multi_play_info.FrameCounts[i];
    const auto src = reinterpret_cast<const INPUT_BITS *>(temp.get());
    this->all_playback_buf.insert(this->all_playback_buf.end(), src, src + n_frames);
    total_frames += n_frames;
  }

  // Copy combined data into this->demo_buffer for DemoplayMove()
  if (total_frames > DEMOBUF_MAX)
    total_frames = DEMOBUF_MAX;
  memcpy(this->demo_buffer.data(), this->all_playback_buf.data(),
         total_frames * sizeof(INPUT_BITS));
  this->demo_info.FrameCount = total_frames;
  this->demo_info.RndSeed = this->multi_play_info.RndSeed;
  this->demo_info.CfgDat = this->multi_play_info.CfgDat;
  this->demo_info.Exp = this->multi_play_info.Exp;
  this->demo_info.Weapon = this->multi_play_info.Weapon;

  this->load_all_enable = true;
  return this->LoadSetup();
}
