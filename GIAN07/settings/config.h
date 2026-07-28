///
/// Config - Config data, split into per-domain sub-configs
///

#pragma once

#include <cstdint>
#include <string>

#include "audio/midi.h"
#include "audio/volume.h"
#include "gameplay/game_rules.h"
#include "gfx/graphics.h"
#include "sys/input.h"

// Limits
constexpr auto kMaxPlayerStock = 6;
constexpr auto kMaxBombStock = 6;
constexpr auto kMaxFpsDivisor = 3;
constexpr auto kRegularStageCount = 6;

struct GameConfig {
  GameLevel game_level = GameLevel::Normal;
  PracticeMode practice_mode = PracticeMode::Off;
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
  GRAPHICS_PARAM_FLAGS graphics_param_flags{};
  uint8_t screenshot_effort = 0;

  [[nodiscard]] GRAPHICS_PARAMS ToParams() const;
  void ApplyParams(const GRAPHICS_PARAMS &params);
};

struct UiConfig {
  bool message_window_upper = false;
  bool messages_disabled = false;
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

  [[nodiscard]] uint8_t PackFlags() const;
  void UnpackFlags(uint8_t value);
};

struct DebugConfig {
  int32_t hitbox_display = 0;
};

struct ConfigData {
  GameConfig game;
  GraphicsConfig graphics;
  UiConfig ui;
  AudioConfig audio;
  InputConfig input;
  DebugConfig debug;

  void Load();
  void Save() const;
};
