///
/// Config - Config data, split into per-domain sub-configs
///

#pragma once

#include <cstdint>
#include <string>

#include "audio/volume.h"
#include "gameplay/game_rules.h"
#include "gfx/graphics.h"
#include "sys/input.h"

enum class MidiVariant : uint8_t;

// Limits
constexpr auto kMaxPlayerStock = 6;
constexpr auto kMaxBombStock = 6;
constexpr auto kMaxFpsDivisor = 3;
constexpr auto kRegularStageCount = 6;

struct GameConfig {
  PracticeMode practice_mode = PracticeMode::Off;
  uint8_t player_stock = 2;
  uint8_t bomb_stock = 2;
  bool show_focus_hitbox = true;
};

enum class DisplayMode : uint8_t {
  Windowed,
  Fullscreen,
};

enum class FullscreenMode : uint8_t {
  Borderless,
  Exclusive,
};

enum class ScalingMode : uint8_t {
  Framebuffer,
  Geometry,
};

struct GraphicsConfig {
  std::string graphics_api;
  DisplayMode display_mode = DisplayMode::Windowed;
  FullscreenMode fullscreen_mode = FullscreenMode::Borderless;
  GRAPHICS_FULLSCREEN_FIT fullscreen_fit = GRAPHICS_FULLSCREEN_FIT::INTEGER;
  ScalingMode scaling_mode = ScalingMode::Framebuffer;
  uint8_t window_scale_4x = 0;
  int16_t window_left = GRAPHICS_TOPLEFT_UNDEFINED;
  int16_t window_top = GRAPHICS_TOPLEFT_UNDEFINED;
  uint8_t fps_divisor = 1;
  uint8_t screenshot_effort = 0;
};

enum class MessageWindowMode : uint8_t {
  Upper,
  Lower,
  Hidden,
};

struct UiConfig {
  MessageWindowMode message_window = MessageWindowMode::Lower;
  std::string language = "ja";
};

struct AudioConfig {
  bool bgm_enabled = true;
  bool se_enabled = true;
  bool bgm_vol_norm = true;
  bool fix_sysex_bugs = true;
  MidiVariant midi_variant{};
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
};

struct DebugConfig {
  int32_t hitbox_display = 0;
  bool demo_recording = false;
};

struct ProgressConfig {
  uint8_t extra_stg_flags = 0;
};

struct ConfigData {
  GameConfig game;
  GraphicsConfig graphics;
  UiConfig ui;
  AudioConfig audio;
  InputConfig input;
  DebugConfig debug;
  ProgressConfig progress;
};

[[nodiscard]] ConfigData LoadConfig();
void SaveConfig(const ConfigData &config);
