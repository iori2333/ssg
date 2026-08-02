///
/// Config - Config data
///

#include <cstdint>
#include <fstream>
#include <ios>
#include <type_traits>
#include <utility>

#include <toml++/toml.hpp>

#include "config.h"

#include "audio/core/audio_types.h"
#include "gameplay/game_rules.h"
#include "gfx/graphics.h"
#include "music/music_player.h"
#include "sys/input.h"
#include "util/enum_flags.h"

namespace {

constexpr auto kConfigFileName = "SSG.TOML";
constexpr auto kMaxLoadedPlayerStock = kMaxPlayerStock + 2;
constexpr auto kMaxLoadedBombStock = kMaxBombStock + 1;
constexpr uint8_t kExtraStageFlagMask = 0x07;

// Validation helpers

constexpr bool ValidPlayerStock(uint8_t v) {
  return v <= kMaxLoadedPlayerStock;
}
constexpr bool ValidBombStock(uint8_t v) {
  return v <= kMaxLoadedBombStock;
}
constexpr bool ValidPracticeMode(PracticeMode v) {
  return std::to_underlying(v) <= std::to_underlying(PracticeMode::Invincible);
}
constexpr bool ValidFPSDivisor(uint8_t v) { return v <= kMaxFpsDivisor; }
constexpr bool ValidScreenshotEffort(uint8_t v) {
  return v <= kScreenshotEffortMax;
}
constexpr bool ValidVolume(audio::Volume v) {
  return v <= audio::kMaxVolume;
}
constexpr bool ValidMidiVariant(MidiVariant v) {
  return v <= MidiVariant::Arranged;
}
constexpr bool ValidWinMMPad(InputPadButton v) { return v <= 32; }
constexpr bool ValidExtraStageFlags(uint8_t v) {
  return (v & ~kExtraStageFlagMask) == 0;
}

namespace {

template <typename T, typename V = decltype([](auto) { return true; })>
void LoadToml(const toml::table &tbl, const char *key, T &dest,
              V &&validate = {}) {
  if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    if (auto val = tbl[key].template value<U>()) {
      auto v = static_cast<T>(*val);
      if (std::forward<V>(validate)(v)) {
        dest = v;
      }
    }
  } else {
    if (auto val = tbl[key].template value<T>()) {
      if (std::forward<V>(validate)(*val)) {
        dest = *val;
      }
    }
  }
}

} // namespace

void TOMLLoad(const char *fn, ConfigData &cfg) {
  std::ifstream file(fn, std::ios::binary);
  if (!file) {
    return;
  }

  toml::table tbl;
  try {
    tbl = toml::parse(file);
  } catch (const toml::parse_error &) {
    return;
  }

  // [difficulty]
  if (auto *sec = tbl["difficulty"].as_table()) {
    LoadToml(*sec, "player_stock", cfg.game.player_stock, ValidPlayerStock);
    LoadToml(*sec, "bomb_stock", cfg.game.bomb_stock, ValidBombStock);
    LoadToml(*sec, "practice_mode", cfg.game.practice_mode, ValidPracticeMode);
    LoadToml(*sec, "show_focus_hitbox", cfg.game.show_focus_hitbox);
  }

  // [graphics]
  if (auto *sec = tbl["graphics"].as_table()) {
    LoadToml(*sec, "api", cfg.graphics.graphics_api);
    LoadToml(*sec, "window_scale_4x", cfg.graphics.window_scale_4x,
             [](auto) { return true; });
    LoadToml(*sec, "window_left", cfg.graphics.window_left);
    LoadToml(*sec, "window_top", cfg.graphics.window_top);
    LoadToml(*sec, "fps_divisor", cfg.graphics.fps_divisor, ValidFPSDivisor);
    GraphicsParamFlags stored_flags{};
    LoadToml(*sec, "graphics_param_flags", stored_flags,
             [](GraphicsParamFlags f) {
               return (std::to_underlying(f) &
                       ~std::to_underlying(GraphicsParamFlags::Mask)) == 0;
             });
    const auto fullscreen =
        GraphicsParams{.flags = stored_flags}.FullscreenFlags();
    cfg.graphics.display_mode =
        fullscreen.fullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
    cfg.graphics.fullscreen_mode = fullscreen.exclusive
                                       ? FullscreenMode::Exclusive
                                       : FullscreenMode::Borderless;
    cfg.graphics.fullscreen_fit = fullscreen.fit;
    cfg.graphics.scaling_mode =
        !!(stored_flags & GraphicsParamFlags::ScaleGeometry)
            ? ScalingMode::Geometry
            : ScalingMode::Framebuffer;
    LoadToml(*sec, "screenshot_effort", cfg.graphics.screenshot_effort,
             ValidScreenshotEffort);

    bool legacy_upper = cfg.ui.message_window == MessageWindowMode::Upper;
    bool legacy_disabled = cfg.ui.message_window == MessageWindowMode::Hidden;
    LoadToml(*sec, "window_upper", legacy_upper);
    LoadToml(*sec, "msg_disable", legacy_disabled);
    auto mode = MessageWindowMode::Lower;
    if (legacy_disabled) {
      mode = MessageWindowMode::Hidden;
    } else if (legacy_upper) {
      mode = MessageWindowMode::Upper;
    }
    cfg.ui.message_window = mode;
  }

  if (auto *sec = tbl["ui"].as_table()) {
    bool upper = cfg.ui.message_window == MessageWindowMode::Upper;
    bool disabled = cfg.ui.message_window == MessageWindowMode::Hidden;
    LoadToml(*sec, "message_window_upper", upper);
    LoadToml(*sec, "messages_disabled", disabled);
    auto mode = MessageWindowMode::Lower;
    if (disabled) {
      mode = MessageWindowMode::Hidden;
    } else if (upper) {
      mode = MessageWindowMode::Upper;
    }
    cfg.ui.message_window = mode;
    LoadToml(*sec, "language", cfg.ui.language);
  }

  // [sound]
  if (auto *sec = tbl["sound"].as_table()) {
    LoadToml(*sec, "bgm_enabled", cfg.audio.bgm_enabled);
    LoadToml(*sec, "se_enabled", cfg.audio.se_enabled);
    LoadToml(*sec, "se_volume", cfg.audio.se_volume, ValidVolume);
    LoadToml(*sec, "bgm_volume", cfg.audio.bgm_volume, ValidVolume);
    LoadToml(*sec, "bgm_pack", cfg.audio.bgm_pack);
    LoadToml(*sec, "soundfont", cfg.audio.soundfont);
    LoadToml(*sec, "midi_variant", cfg.audio.midi_variant, ValidMidiVariant);
    bool midi_fix = cfg.audio.fix_sysex_bugs;
    LoadToml(*sec, "midi_fix_sysex_bugs", midi_fix);
    cfg.audio.fix_sysex_bugs = midi_fix;
  }

  // [input]
  if (auto *sec = tbl["input"].as_table()) {
    LoadToml(*sec, "joypad_enabled", cfg.input.joypad_enabled);
    LoadToml(*sec, "z_msg_skip_enabled", cfg.input.z_msg_skip_enabled);
    LoadToml(*sec, "z_spd_down_enabled", cfg.input.z_spd_down_enabled);
    LoadToml(*sec, "pad_tama", cfg.input.pad_tama, ValidWinMMPad);
    LoadToml(*sec, "pad_bomb", cfg.input.pad_bomb, ValidWinMMPad);
    LoadToml(*sec, "pad_shift", cfg.input.pad_shift, ValidWinMMPad);
    LoadToml(*sec, "pad_cancel", cfg.input.pad_cancel, ValidWinMMPad);
  }

  // [progress]
  if (auto *sec = tbl["progress"].as_table()) {
    LoadToml(*sec, "extra_stg_flags", cfg.progress.extra_stg_flags,
             ValidExtraStageFlags);
  }
}

void TOMLSave(const char *fn, const ConfigData &cfg) {
  toml::table tbl;

  // [difficulty]
  {
    toml::table sec;
    sec.emplace("player_stock", cfg.game.player_stock);
    sec.emplace("bomb_stock", cfg.game.bomb_stock);
    sec.emplace("practice_mode", std::to_underlying(cfg.game.practice_mode));
    sec.emplace("show_focus_hitbox", cfg.game.show_focus_hitbox);
    tbl.emplace("difficulty", std::move(sec));
  }

  // [graphics]
  {
    toml::table sec;
    sec.emplace("api", cfg.graphics.graphics_api);
    sec.emplace("window_scale_4x", cfg.graphics.window_scale_4x);
    sec.emplace("window_left", cfg.graphics.window_left);
    sec.emplace("window_top", cfg.graphics.window_top);
    sec.emplace("fps_divisor", cfg.graphics.fps_divisor);
    GraphicsParamFlags flags{};
    if (cfg.graphics.display_mode == DisplayMode::Fullscreen) {
      flags |= GraphicsParamFlags::Fullscreen;
    }
    if (cfg.graphics.fullscreen_mode == FullscreenMode::Exclusive) {
      flags |= GraphicsParamFlags::FullscreenExclusive;
    }
    if (cfg.graphics.scaling_mode == ScalingMode::Geometry) {
      flags |= GraphicsParamFlags::ScaleGeometry;
    }
    SetEnumFlag(flags, GraphicsParamFlags::FullscreenFit,
                std::to_underlying(cfg.graphics.fullscreen_fit));
    sec.emplace("graphics_param_flags", std::to_underlying(flags));
    sec.emplace("screenshot_effort", cfg.graphics.screenshot_effort);
    tbl.emplace("graphics", std::move(sec));
  }

  // [ui]
  {
    toml::table sec;
    sec.emplace("message_window_upper",
                cfg.ui.message_window == MessageWindowMode::Upper);
    sec.emplace("messages_disabled",
                cfg.ui.message_window == MessageWindowMode::Hidden);
    sec.emplace("language", cfg.ui.language);
    tbl.emplace("ui", std::move(sec));
  }

  // [sound]
  {
    toml::table sec;
    sec.emplace("bgm_enabled", cfg.audio.bgm_enabled);
    sec.emplace("se_enabled", cfg.audio.se_enabled);
    sec.emplace("se_volume", cfg.audio.se_volume);
    sec.emplace("bgm_volume", cfg.audio.bgm_volume);
    sec.emplace("bgm_pack", cfg.audio.bgm_pack);
    sec.emplace("soundfont", cfg.audio.soundfont);
    sec.emplace("midi_variant", std::to_underlying(cfg.audio.midi_variant));
    sec.emplace("midi_fix_sysex_bugs", cfg.audio.fix_sysex_bugs);
    tbl.emplace("sound", std::move(sec));
  }

  // [input]
  {
    toml::table sec;
    sec.emplace("joypad_enabled", cfg.input.joypad_enabled);
    sec.emplace("z_msg_skip_enabled", cfg.input.z_msg_skip_enabled);
    sec.emplace("z_spd_down_enabled", cfg.input.z_spd_down_enabled);
    sec.emplace("pad_tama", cfg.input.pad_tama);
    sec.emplace("pad_bomb", cfg.input.pad_bomb);
    sec.emplace("pad_shift", cfg.input.pad_shift);
    sec.emplace("pad_cancel", cfg.input.pad_cancel);
    tbl.emplace("input", std::move(sec));
  }

  // [progress]
  {
    toml::table sec;
    sec.emplace("extra_stg_flags", cfg.progress.extra_stg_flags);
    tbl.emplace("progress", std::move(sec));
  }

  std::ofstream file(fn, std::ios::binary | std::ios::trunc);
  if (!file) {
    return;
  }
  file << tbl;
}

} // namespace

ConfigData LoadConfig() {
  ConfigData config;
  TOMLLoad(kConfigFileName, config);
  return config;
}

void SaveConfig(const ConfigData &config) { TOMLSave(kConfigFileName, config); }
