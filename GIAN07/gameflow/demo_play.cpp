///
/// DemoPlay - Demo playback processing
///

#include <algorithm>
#include <chrono>
#include <format>
#include <utility>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>

#include "demo_manager.h"
#include "demo_play.h"

#include "core/config.h"
#include "core/game_manager.h"
#include "core/gian.h"
#include "core/lz_uty.h"
#include "data/stage_manager.h"
#include "gameflow/gameflow_manager.h"
#include "player/player.h"
#include "sys/file.h"
#include "sys/input.h"
#include "util/guard.h"
#include "util/ut_math.h"

// File-static variables moved to DemoManager struct in demo_manager.h

namespace {
std::string ReplayAllFN(bool exstg) {
  const auto *prefix = exstg ? "replay_ex" : "replay";
  const auto now = std::chrono::system_clock::now();
  const auto local = std::chrono::floor<std::chrono::seconds>(now);
  return std::format("{}_{:%Y%m%d_%H%M%S}.DAT", prefix, local);
}
} // namespace

void DemoManager::Init() {
  // Prepare random seed
  demo_info.RndSeed =
      ((Cast::up<uint32_t>(rnd()) + 1U) * (Cast::up<uint32_t>(rnd()) + 1U));
  rnd_seed_set(demo_info.RndSeed);

  demo_info.Exp = Players.Power();
  demo_info.Weapon = Players.Weapon();
  demo_info.CfgDat.GameLevel = std::to_underlying(GameFlow.ctx.game.level);
  demo_info.CfgDat.PlayerStock = Players.Lives();
  demo_info.CfgDat.BombStock = ConfigDat.bomb_stock;
  demo_info.CfgDat.InputFlags = ConfigDat.PackInputFlags();

  demo_frame_cur = 0;
  save_all_enable = true;
  multi_stage_count = 0;
  stage_record_bufs.clear();
}

bool DemoManager::HasRecordedStages() const {
  return (save_all_enable && (multi_stage_count > 0 || demo_frame_cur > 0));
}

void DemoManager::FlushStage() {
  if (!save_all_enable) {
    return;
  }
  if (demo_frame_cur == 0) {
    return;
  }

  if (multi_stage_count < REPLAY_STAGE_MAX) {
    multi_stage_nums[multi_stage_count] = std::to_underlying(GameFlow.ctx.game.stage);
    multi_stage_frames[multi_stage_count] = demo_frame_cur;

    std::vector<INPUT_BITS> stage_data(demo_buffer.data(),
                                       demo_buffer.data() + demo_frame_cur);
    stage_record_bufs.push_back(std::move(stage_data));

    multi_stage_count++;
  }

  demo_frame_cur = 0;
}

bool DemoManager::LoadSetup() {
  demo_frame_cur = 0;
  load_enable = true;

  // Initialize config
  // Preserve current config
  config_temp.PlayerStock = ConfigDat.player_stock;
  config_temp.BombStock = ConfigDat.bomb_stock;
  config_temp.InputFlags = ConfigDat.PackInputFlags();

  // Transfer recorded config
  ConfigDat.bomb_stock = demo_info.CfgDat.BombStock;
  ConfigDat.player_stock = demo_info.CfgDat.PlayerStock;
  ConfigDat.UnpackInputFlags(demo_info.CfgDat.InputFlags);
  GameFlow.ctx.game.level = static_cast<GameLevel>(demo_info.CfgDat.GameLevel);

  // Restore player stats
  Players.ApplyReplayState(demo_info.Weapon, demo_info.Exp,
                           ConfigDat.player_stock, ConfigDat.bomb_stock);

  // Initialize random number
  // Sync random seed last
  rnd_seed_set(demo_info.RndSeed);

  return true;
}

bool DemoManager::Record(INPUT_BITS key) {
  if (!save_all_enable) {
    return false;
  }

  demo_buffer[demo_frame_cur++] = key;

  // Buffer full or ESC pressed
  if ((demo_frame_cur == DEMOBUF_MAX) || ((key & KEY_ESC) != 0)) {
    demo_frame_cur--;
    return true;
  }
  return false;
}

void DemoManager::SaveDemo() {
  if (!save_all_enable) {
    return;
  }

  demo_buffer[demo_frame_cur] = KEY_ESC;
  demo_info.FrameCount = (demo_frame_cur + 1);

  char fn[] = "STG_Demo.DAT";
  fn[3] = ('0' + static_cast<int>(GameFlow.ctx.game.stage));

  auto *f = SDL_IOFromFile(fn, "wb");
  if (f != nullptr) {
    auto f_guard = make_guard(f, SDL_CloseIO);
    SDL_WriteIO(f, &demo_info, sizeof(demo_info));
    SDL_WriteIO(f, demo_buffer.data(),
                (sizeof(demo_buffer[0]) * demo_info.FrameCount));
  }
}

bool DemoManager::LoadDemo(GameStage stage) {
  // Unpack
  const auto temp = stage_mgr.LoadDemo(static_cast<int>(std::to_underlying(stage)));
  auto temp_cursor = temp.cursor();
  {
    const auto maybe_info = temp_cursor.next<DemoPlayState>();
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
    memcpy(demo_buffer.data(), inputs.data(), inputs.size());
  }
  return LoadSetup();
}

INPUT_BITS DemoManager::Move() {
  if (!load_enable) {
    return KEY_ESC;
  }

  const auto ptr = demo_frame_cur;
  if (ptr >= demo_info.FrameCount) {
    load_enable = false;
    return KEY_ESC;
  }

  demo_frame_cur++;
  return demo_buffer[ptr];
}

void DemoManager::Cleanup() {
  ConfigDat.player_stock = config_temp.PlayerStock;
  ConfigDat.bomb_stock = config_temp.BombStock;
  ConfigDat.UnpackInputFlags(config_temp.InputFlags);

  load_enable = false;
  load_all_enable = false;
}

void DemoManager::SaveReplayAll(bool exstg) {
  if (!save_all_enable) {
    return;
  }

  // Flush current stage data if any (not yet flushed by stage clear)
  if (demo_frame_cur > 0 && multi_stage_count < REPLAY_STAGE_MAX) {
    multi_stage_nums[multi_stage_count] = std::to_underlying(GameFlow.ctx.game.stage);
    multi_stage_frames[multi_stage_count] = demo_frame_cur;
    stage_record_bufs.emplace_back(demo_buffer.data(),
                                   demo_buffer.data() + demo_frame_cur);
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
  for (int i = 0; std::cmp_less(i, multi_stage_count); i++) {
    info.Stages[i] = multi_stage_nums[i];
    info.FrameCounts[i] = multi_stage_frames[i];
  }

  PackWriter out;
  out.Add(std::span<const uint8_t>(
      reinterpret_cast<const uint8_t *>(&info), sizeof(info)));
  for (auto &buf : stage_record_bufs) {
    out.Add(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t *>(buf.data()),
        buf.size() * sizeof(INPUT_BITS)));
  }

  const auto fn = ReplayAllFN(exstg);
  out.Write(fn.c_str());

  stage_record_bufs.clear();
  multi_stage_count = 0;
}

bool DemoManager::LoadReplayAll(const char *fn) {
  const auto in = PackFile::Open(fn);
  if (!in) {
    return false;
  }

  BYTE_BUFFER_OWNED temp = in.Extract(0);
  if (nullptr == temp) {
    return false;
  }
  memcpy(&multi_play_info, temp.get(), sizeof(MULTI_REPLAY_INFO));

  // Compute max stage for stage transition gating
  playback_max_stage = GameStage::NONE;
  for (uint8_t i = 0; i < multi_play_info.StageCount; i++) {
    playback_max_stage =
        std::max(static_cast<GameStage>(multi_play_info.Stages[i]),
                 playback_max_stage);
  }

  all_playback_buf.clear();
  uint32_t total_frames = 0;
  for (uint8_t i = 0; i < multi_play_info.StageCount; i++) {
    temp = in.Extract(i + 1);
    if (nullptr == temp) {
      return false;
    }
    uint32_t n_frames = multi_play_info.FrameCounts[i];
    const auto *const src = reinterpret_cast<const INPUT_BITS *>(temp.get());
    all_playback_buf.insert(all_playback_buf.end(), src, src + n_frames);
    total_frames += n_frames;
  }

  // Copy combined data into demo_buffer for DemoplayMove()
  total_frames = std::min<uint32_t>(total_frames, DEMOBUF_MAX);
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
