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
  demo_info.RndSeed =
      ((Cast::up<uint32_t>(rnd()) + 1u) * (Cast::up<uint32_t>(rnd()) + 1u));
  rnd_seed_set(demo_info.RndSeed);

  demo_info.Exp = Players.viv.exp;
  demo_info.Weapon = Players.viv.weapon;
  demo_info.CfgDat.GameLevel = GameState.game_level;
  demo_info.CfgDat.PlayerStock = Players.viv.left;
  demo_info.CfgDat.BombStock = ConfigDat.BombStock.v;
  demo_info.CfgDat.InputFlags = ConfigDat.InputFlags.v;

  demo_frame_cur = 0;
  save_all_enable = true;
  multi_stage_count = 0;
  stage_record_bufs.clear();
}

bool DemoManager::HasRecordedStages(void) {
  return (save_all_enable && (multi_stage_count > 0 || demo_frame_cur > 0));
}

void DemoManager::FlushStage(void) {
  if (!save_all_enable)
    return;
  if (demo_frame_cur == 0)
    return;

  if (multi_stage_count < REPLAY_STAGE_MAX) {
    multi_stage_nums[multi_stage_count] = GameState.game_stage;
    multi_stage_frames[multi_stage_count] = demo_frame_cur;

    std::vector<INPUT_BITS> stage_data(demo_buffer.data(), demo_buffer.data() + demo_frame_cur);
    stage_record_bufs.push_back(std::move(stage_data));

    multi_stage_count++;
  }

  demo_frame_cur = 0;
}

bool DemoManager::LoadSetup() {
  demo_frame_cur = 0;
  load_enable = true;

  // コンフィグの初期化 //
  // 現在のコンフィグを保持する //
  config_temp.PlayerStock = ConfigDat.PlayerStock.v;
  config_temp.BombStock = ConfigDat.BombStock.v;
  config_temp.InputFlags = ConfigDat.InputFlags.v;

  // そのときのコンフィグを転送 //
  ConfigDat.BombStock.v = demo_info.CfgDat.BombStock;
  ConfigDat.PlayerStock.v = demo_info.CfgDat.PlayerStock;
  ConfigDat.InputFlags.v = demo_info.CfgDat.InputFlags;
  GameState.game_level = demo_info.CfgDat.GameLevel;

  // 本体の性能記述 //
  Players.viv.exp = demo_info.Exp;
  Players.viv.weapon = demo_info.Weapon;
  Players.viv.left = ConfigDat.PlayerStock.v;
  Players.viv.bomb = ConfigDat.BombStock.v;

  // 乱数の初期化 //
  // 最後に乱数もそろえる //
  rnd_seed_set(demo_info.RndSeed);

  return true;
}

bool DemoManager::Record(INPUT_BITS key) {
  if (!save_all_enable) {
    return false;
  }

  demo_buffer[demo_frame_cur++] = key;

  // バッファが最後に来たか、ＥＳＣが押された場合 //
  if ((demo_frame_cur == DEMOBUF_MAX) || (key & KEY_ESC)) {
    demo_frame_cur--;
    return true;
  }
  return false;
}

void DemoManager::SaveDemo(void) {
  if (!save_all_enable)
    return;

  demo_buffer[demo_frame_cur] = KEY_ESC;
  demo_info.FrameCount = (demo_frame_cur + 1);

  char8_t fn[] = u8"STG_Demo.DAT";
  fn[3] = ('0' + GameState.game_stage);

  auto *f = SDL_IOFromFile(fn, "wb");
  if (f) {
    SDL_WriteIO(f, &demo_info, sizeof(demo_info));
    SDL_WriteIO(f, demo_buffer.data(), (sizeof(demo_buffer[0]) * demo_info.FrameCount));
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
    demo_info = maybe_info.value()[0];
  }
  {
    const auto maybe_inputs = temp_cursor.next<uint16_t>(demo_info.FrameCount);
    if (!maybe_inputs) {
      return false;
    }
    const auto inputs = maybe_inputs.value();
    memcpy(demo_buffer.data(), inputs.data(), inputs.size_bytes());
  }
  return LoadSetup();
}

INPUT_BITS DemoManager::Move(void) {
  if (!load_enable)
    return KEY_ESC;

  const auto ptr = demo_frame_cur;
  if (ptr >= demo_info.FrameCount) {
    load_enable = false;
    return KEY_ESC;
  }

  demo_frame_cur++;
  return demo_buffer[ptr];
}

void DemoManager::Cleanup(void) {
  ConfigDat.PlayerStock.v = config_temp.PlayerStock;
  ConfigDat.BombStock.v = config_temp.BombStock;
  ConfigDat.InputFlags.v = config_temp.InputFlags;

  load_enable = false;
  load_all_enable = false;
}

void DemoManager::SaveReplayAll(bool exstg) {
  if (!save_all_enable)
    return;

  // Flush current stage data if any (not yet flushed by stage clear)
  if (demo_frame_cur > 0 && multi_stage_count < REPLAY_STAGE_MAX) {
    multi_stage_nums[multi_stage_count] = GameState.game_stage;
    multi_stage_frames[multi_stage_count] = demo_frame_cur;
    stage_record_bufs.emplace_back(demo_buffer.data(), demo_buffer.data() + demo_frame_cur);
    multi_stage_count++;
    demo_frame_cur = 0;
  }

  save_all_enable = false;

  MULTI_REPLAY_INFO info = {};
  info.RndSeed = demo_info.RndSeed;
  info.StageCount = multi_stage_count;
  info.CfgDat = demo_info.CfgDat;
  info.Exp = demo_info.Exp;
  info.Weapon = demo_info.Weapon;
  for (int i = 0; i < multi_stage_count; i++) {
    info.Stages[i] = multi_stage_nums[i];
    info.FrameCounts[i] = multi_stage_frames[i];
  }

  PACKFILE_WRITE out;
  out.files.push_back(
      {reinterpret_cast<const uint8_t *>(&info), sizeof(info)});
  for (auto &buf : stage_record_bufs) {
    out.files.push_back(
        {reinterpret_cast<const uint8_t *>(buf.data()),
         buf.size() * sizeof(INPUT_BITS)});
  }

  const auto fn = ReplayAllFN(exstg);
  out.Write(fn.c_str());

  stage_record_bufs.clear();
  multi_stage_count = 0;
}

bool DemoManager::LoadReplayAll(const char8_t *fn) {
  const auto in = FilStartR(fn);
  if (!in)
    return false;

  BYTE_BUFFER_OWNED temp = in.MemExpand(0);
  if (nullptr == temp)
    return false;
  memcpy(&multi_play_info, temp.get(), sizeof(MULTI_REPLAY_INFO));

  // Compute max stage for stage transition gating
  playback_max_stage = 0;
  for (uint8_t i = 0; i < multi_play_info.StageCount; i++) {
    if (multi_play_info.Stages[i] > playback_max_stage)
      playback_max_stage = multi_play_info.Stages[i];
  }

  all_playback_buf.clear();
  uint32_t total_frames = 0;
  for (uint8_t i = 0; i < multi_play_info.StageCount; i++) {
    temp = in.MemExpand(i + 1);
    if (nullptr == temp)
      return false;
    uint32_t n_frames = multi_play_info.FrameCounts[i];
    const auto src = reinterpret_cast<const INPUT_BITS *>(temp.get());
    all_playback_buf.insert(all_playback_buf.end(), src, src + n_frames);
    total_frames += n_frames;
  }

  // Copy combined data into demo_buffer for DemoplayMove()
  if (total_frames > DEMOBUF_MAX)
    total_frames = DEMOBUF_MAX;
  memcpy(demo_buffer.data(), all_playback_buf.data(),
         total_frames * sizeof(INPUT_BITS));
  demo_info.FrameCount = total_frames;
  demo_info.RndSeed = multi_play_info.RndSeed;
  demo_info.CfgDat = multi_play_info.CfgDat;
  demo_info.Exp = multi_play_info.Exp;
  demo_info.Weapon = multi_play_info.Weapon;

  load_all_enable = true;
  return LoadSetup();
}
