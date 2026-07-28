///
/// Config - Config data
///

#include <sstream>
#include <type_traits>
#include <utility>

#include <SDL3/SDL_iostream.h>
#include <toml++/toml.hpp>

#include "config.h"

#include "gfx/graphics_backend.h"
#include "sys/file.h"
#include "util/guard.h"

static constexpr auto CFG_FN = "SSG.TOML";
static constexpr auto kMaxLoadedPlayerStock = kMaxPlayerStock + 2;
static constexpr auto kMaxLoadedBombStock = kMaxBombStock + 1;

// Validation helpers

static constexpr bool ValidGameLevel(GameLevel v) {
  return std::to_underlying(v) <= std::to_underlying(GameLevel::Lunatic);
}
static constexpr bool ValidPlayerStock(uint8_t v) {
  return v <= kMaxLoadedPlayerStock;
}
static constexpr bool ValidBombStock(uint8_t v) {
  return v <= kMaxLoadedBombStock;
}
static constexpr bool ValidPracticeMode(PracticeMode v) {
  return std::to_underlying(v) <= std::to_underlying(PracticeMode::Invincible);
}
static constexpr bool ValidFPSDivisor(uint8_t v) { return v <= kMaxFpsDivisor; }
static constexpr bool ValidScreenshotEffort(uint8_t v) {
  return v <= GRP_SCREENSHOT_EFFORT_MAX;
}
static constexpr bool ValidVolume(VOLUME v) { return v <= VOLUME_MAX; }
static constexpr bool ValidWinMMPad(INPUT_PAD_BUTTON v) { return v <= 32; }

namespace {

template <typename T, typename V = decltype([](auto) { return true; })>
void LoadToml(const toml::table &tbl, const char *key, T &dest,
              V &&validate = {}) {
  if constexpr (std::is_enum_v<T>) {
    using U = std::underlying_type_t<T>;
    if (auto val = tbl[key].template value<U>()) {
      auto v = static_cast<T>(*val);
      if (validate(v))
        dest = v;
    }
  } else {
    if (auto val = tbl[key].template value<T>()) {
      if (validate(*val))
        dest = *val;
    }
  }
}

} // namespace

static void TOMLLoad(const char *fn, ConfigData &cfg) {
  auto *f = SDL_IOFromFile(fn, "rb");
  if (f == nullptr) {
    return;
  }
  auto f_guard = make_guard(f, SDL_CloseIO);

  const auto size = SDL_GetIOSize(f);
  if (size <= 0) {
    return;
  }

  std::string buf(static_cast<size_t>(size), '\0');
  if (SDL_ReadIO(f, buf.data(), size) != size) {
    return;
  }

  toml::table tbl;
  try {
    tbl = toml::parse(buf);
  } catch (const toml::parse_error &) {
    return;
  }

  // [difficulty]
  if (auto *sec = tbl["difficulty"].as_table()) {
    LoadToml(*sec, "game_level", cfg.game.game_level, ValidGameLevel);
    LoadToml(*sec, "player_stock", cfg.game.player_stock, ValidPlayerStock);
    LoadToml(*sec, "bomb_stock", cfg.game.bomb_stock, ValidBombStock);
    LoadToml(*sec, "practice_mode", cfg.game.practice_mode, ValidPracticeMode);
  }

  // [graphics]
  if (auto *sec = tbl["graphics"].as_table()) {
    LoadToml(*sec, "device_id", cfg.graphics.device_id,
             [](auto) { return true; });
    LoadToml(*sec, "api", cfg.graphics.graphics_api);
    LoadToml(*sec, "window_scale_4x", cfg.graphics.window_scale_4x,
             [](auto) { return true; });
    LoadToml(*sec, "window_left", cfg.graphics.window_left);
    LoadToml(*sec, "window_top", cfg.graphics.window_top);
    LoadToml(*sec, "fps_divisor", cfg.graphics.fps_divisor, ValidFPSDivisor);
    LoadToml(*sec, "window_upper", cfg.ui.message_window_upper);
    LoadToml(*sec, "msg_disable", cfg.ui.messages_disabled);
    LoadToml(*sec, "graphics_param_flags", cfg.graphics.graphics_param_flags,
             [](GRAPHICS_PARAM_FLAGS f) {
               return (std::to_underlying(f) &
                       ~std::to_underlying(GRAPHICS_PARAM_FLAGS::MASK)) == 0;
             });
    LoadToml(*sec, "screenshot_effort", cfg.graphics.screenshot_effort,
             ValidScreenshotEffort);
  }

  if (auto *sec = tbl["ui"].as_table()) {
    LoadToml(*sec, "message_window_upper", cfg.ui.message_window_upper);
    LoadToml(*sec, "messages_disabled", cfg.ui.messages_disabled);
  }

  // [sound]
  if (auto *sec = tbl["sound"].as_table()) {
    LoadToml(*sec, "bgm_enabled", cfg.audio.bgm_enabled);
    LoadToml(*sec, "se_enabled", cfg.audio.se_enabled);
    LoadToml(*sec, "bgm_volume_normalized", cfg.audio.bgm_vol_norm);
    LoadToml(*sec, "se_volume", cfg.audio.se_volume, ValidVolume);
    LoadToml(*sec, "bgm_volume", cfg.audio.bgm_volume, ValidVolume);
    LoadToml(*sec, "bgm_pack", cfg.audio.bgm_pack);
    LoadToml(*sec, "soundfont", cfg.audio.soundfont);
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
}

static void TOMLSave(const char *fn, const ConfigData &cfg) {
  toml::table tbl;

  // [difficulty]
  {
    toml::table sec;
    sec.emplace("game_level", std::to_underlying(cfg.game.game_level));
    sec.emplace("player_stock", cfg.game.player_stock);
    sec.emplace("bomb_stock", cfg.game.bomb_stock);
    sec.emplace("practice_mode", std::to_underlying(cfg.game.practice_mode));
    tbl.emplace("difficulty", std::move(sec));
  }

  // [graphics]
  {
    toml::table sec;
    sec.emplace("device_id", cfg.graphics.device_id);
    sec.emplace("api", cfg.graphics.graphics_api);
    sec.emplace("window_scale_4x", cfg.graphics.window_scale_4x);
    sec.emplace("window_left", cfg.graphics.window_left);
    sec.emplace("window_top", cfg.graphics.window_top);
    sec.emplace("fps_divisor", cfg.graphics.fps_divisor);
    sec.emplace("graphics_param_flags",
                std::to_underlying(cfg.graphics.graphics_param_flags));
    sec.emplace("screenshot_effort", cfg.graphics.screenshot_effort);
    tbl.emplace("graphics", std::move(sec));
  }

  // [ui]
  {
    toml::table sec;
    sec.emplace("message_window_upper", cfg.ui.message_window_upper);
    sec.emplace("messages_disabled", cfg.ui.messages_disabled);
    tbl.emplace("ui", std::move(sec));
  }

  // [sound]
  {
    toml::table sec;
    sec.emplace("bgm_enabled", cfg.audio.bgm_enabled);
    sec.emplace("se_enabled", cfg.audio.se_enabled);
    sec.emplace("bgm_volume_normalized", cfg.audio.bgm_vol_norm);
    sec.emplace("se_volume", cfg.audio.se_volume);
    sec.emplace("bgm_volume", cfg.audio.bgm_volume);
    sec.emplace("bgm_pack", cfg.audio.bgm_pack);
    sec.emplace("soundfont", cfg.audio.soundfont);
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

  std::ostringstream oss;
  oss << tbl;

  auto *f = SDL_IOFromFile(fn, "wb");
  if (f == nullptr) {
    return;
  }
  auto f_guard = make_guard(f, SDL_CloseIO);

  const auto str = oss.str();
  SDL_MustWriteIO(f, str.data(), str.size());
}

ConfigData LoadConfig() {
  ConfigData config;
  TOMLLoad(CFG_FN, config);
  return config;
}

void SaveConfig(const ConfigData &config) { TOMLSave(CFG_FN, config); }
