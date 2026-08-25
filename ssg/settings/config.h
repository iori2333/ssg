///
/// Config - Config data, split into per-domain sub-configs
///

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "audio/core/audio_types.h"
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
  int player_stock = 2;
  int bomb_stock = 2;
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
  GraphicsFullscreenFit fullscreen_fit = GraphicsFullscreenFit::Integer;
  ScalingMode scaling_mode = ScalingMode::Framebuffer;
  int window_scale_4x = 0;
  int window_left = kGraphicsTopleftUndefined;
  int window_top = kGraphicsTopleftUndefined;
  int fps_divisor = 1;
  int screenshot_effort = 0;
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
  bool fix_sysex_bugs = true;
  MidiVariant midi_variant{};
  std::string soundfont;
  audio::Volume se_volume = ((audio::kMaxVolume * 4) / 10);
  audio::Volume bgm_volume = ((audio::kMaxVolume * 4) / 10);
  std::string bgm_pack;
};

struct InputConfig {
  bool joypad_enabled = false;
  bool z_msg_skip_enabled = true;
  bool z_spd_down_enabled = false;
  InputPadButton pad_tama = 1;
  InputPadButton pad_bomb = 2;
  InputPadButton pad_shift = 0;
  InputPadButton pad_cancel = 0;
};

struct DebugConfig {
  int hitbox_display = 0;
  bool demo_recording = false;
};

enum class ExtraStageFlag : uint8_t {
  None = 0,
  Wide = 1 << 0,
  Homing = 1 << 1,
  Laser = 1 << 2,
  Mask = 0x07,
};

[[nodiscard]] constexpr ExtraStageFlag
ExtraStageFlagForIndex(std::size_t index) {
  return static_cast<ExtraStageFlag>(1U << index);
}

[[nodiscard]] constexpr bool HasExtraStageFlag(ExtraStageFlag flags,
                                               ExtraStageFlag flag) {
  return (std::to_underlying(flags) & std::to_underlying(flag)) != 0;
}

inline constexpr void SetExtraStageFlag(ExtraStageFlag &flags,
                                        ExtraStageFlag flag) {
  flags = static_cast<ExtraStageFlag>(std::to_underlying(flags) |
                                      std::to_underlying(flag));
}

struct ProgressConfig {
  ExtraStageFlag extra_stg_flags = ExtraStageFlag::None;
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
// Returns false (and logs) if the config could not be written.
[[nodiscard]] bool SaveConfig(const ConfigData &config);
