///
/// Config - Config data
///

#pragma once

#include "game/graphics.h"
#include "game/input.h"
#include "game/midi.h"
#include "game/volume.h"
#include "level.h"

// Limits
constexpr auto STOCK_PLAYER_MAX = 6;
constexpr auto STOCK_BOMB_MAX = 6;
constexpr auto FPS_DIVISOR_MAX = 3;
constexpr auto STAGE_MAX = 6; // Number of stages

enum class PracticeMode : uint8_t {
  OFF = 0,
  AUTOBOMB = 1,
  INVINCIBLE = 2,
};

struct ConfigData {
  // Difficulty settings

  GameLevel game_level = GameLevel::NORMAL;
  uint8_t player_stock = 2;
  uint8_t bomb_stock = 2;
  PracticeMode practice_mode = PracticeMode::OFF;

  // Graphics settings
  uint8_t device_id = 0; // Device index
  std::string graphics_api;
  uint8_t window_scale_4x = 0;
  int16_t window_left = GRAPHICS_TOPLEFT_UNDEFINED;
  int16_t window_top = GRAPHICS_TOPLEFT_UNDEFINED;
  BITDEPTH bit_depth; // Bit depth

  uint8_t fps_divisor = 1;

  bool window_upper = false; // Show message window at top
  bool msg_disable = false;  // Skip dialogue scenes

  GRAPHICS_PARAM_FLAGS graphics_param_flags{};

  uint8_t screenshot_effort = 0;

  // Sound/BGM settings
  bool bgm_enabled = true;  // BGM enabled
  bool se_enabled = true;   // Sound effects enabled
  bool bgm_vol_norm = true; // BGM volume normalization enabled
  MID_FLAGS midi_flags = MID_FLAGS::FIX_SYSEX_BUGS;
  VOLUME se_volume = ((VOLUME_MAX * 4) / 10);
  VOLUME bgm_volume = ((VOLUME_MAX * 4) / 10);
  std::string bgm_pack;

  // Input settings
  bool joypad_enabled = false;     // Joypad enabled
  bool z_msg_skip_enabled = true;  // Z key advances message
  bool z_spd_down_enabled = false; // Hold for shift movement
  INPUT_PAD_BUTTON pad_tama = 1;
  INPUT_PAD_BUTTON pad_bomb = 2;
  INPUT_PAD_BUTTON pad_shift = 0;
  INPUT_PAD_BUTTON pad_cancel = 0;

  // Extra stage progress flags
  uint8_t extra_stg_flags = 0;

  // Placed here intentionally (outside checksum range)
  uint8_t stage_select = 0;

  [[nodiscard]] GRAPHICS_PARAMS GraphicsParams() const;
  void GraphicsParamsApply(const GRAPHICS_PARAMS &params);

  [[nodiscard]] uint8_t PackInputFlags() const;
  void UnpackInputFlags(uint8_t v);

  void Load();
  void Save();
};

extern ConfigData ConfigDat;

#ifdef PBG_DEBUG
struct DebugData {
  int32_t MsgDisplay; // Show debug info
  int32_t Hit;        // Hit detection on/off
  int32_t DemoSave;   // Save demo replay

  uint8_t StgSelect; // Stage select (starting stage)
};

extern DebugData DebugDat;
#endif
