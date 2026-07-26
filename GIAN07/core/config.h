///
/// Config - Config data, split into per-domain sub-configs
///

#pragma once

#include <cstdint>
#include <string>

#include "level.h"

#include "audio/midi.h"
#include "audio/volume.h"
#include "gfx/graphics.h"
#include "sys/input.h"

// Limits
constexpr auto STOCK_PLAYER_MAX = 6;
constexpr auto STOCK_BOMB_MAX = 6;
constexpr auto FPS_DIVISOR_MAX = 3;
constexpr auto STAGE_MAX = 6;

enum class PracticeMode : uint8_t {
  OFF = 0,
  AUTOBOMB = 1,
  INVINCIBLE = 2,
};

struct GameConfig {
  GameLevel game_level = GameLevel::NORMAL;
  PracticeMode practice_mode = PracticeMode::OFF;
  uint8_t player_stock = 2;
  uint8_t bomb_stock = 2;
};

struct GraphicsConfig {
  uint8_t device_id = 0;
  std::string graphics_api;
  uint8_t window_scale_4x = 0;
  int16_t window_left = GRAPHICS_TOPLEFT_UNDEFINED;
  int16_t window_top = GRAPHICS_TOPLEFT_UNDEFINED;
  uint8_t fps_divisor = 1;
  bool window_upper = false;
  bool msg_disable = false;
  GRAPHICS_PARAM_FLAGS graphics_param_flags{};
  uint8_t screenshot_effort = 0;

  [[nodiscard]] GRAPHICS_PARAMS GraphicsParams() const;
  void GraphicsParamsApply(const GRAPHICS_PARAMS &params);
};

struct AudioConfig {
  bool bgm_enabled = true;
  bool se_enabled = true;
  bool bgm_vol_norm = true;
  bool fix_sysex_bugs = true;
  std::string soundfont;
  VOLUME se_volume = ((VOLUME_MAX * 4) / 10);
  VOLUME bgm_volume = ((VOLUME_MAX * 4) / 10);
  std::string bgm_pack;
};

struct InputConfig {
  bool joypad_enabled = false;
  bool z_msg_skip_enabled = true;
  bool z_spd_down_enabled = false;
  INPUT_PAD_BUTTON pad_tama = 1;
  INPUT_PAD_BUTTON pad_bomb = 2;
  INPUT_PAD_BUTTON pad_shift = 0;
  INPUT_PAD_BUTTON pad_cancel = 0;

  [[nodiscard]] uint8_t PackInputFlags() const;
  void UnpackInputFlags(uint8_t v);
};

struct DebugConfig {
  int32_t hitbox_display = 0;
};

struct ConfigData {
  GameConfig game;
  GraphicsConfig graphics;
  AudioConfig audio;
  InputConfig input;
  DebugConfig debug;

  void Load();
  void Save();
};

ConfigData LoadConfigFile();
void SaveConfigFile(ConfigData &cfg);
