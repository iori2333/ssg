///
/// Config - Config data
///

#include <SDL3/SDL_iostream.h>

#include "config.h"
#include "game/bgm.h"
#include "game/guard.h"
#include "game/file.h"
#include "game/window_backend.h"
#include <toml++/toml.hpp>

///// Constants /////
static constexpr auto CFG_FN = "SSG.TOML";

#ifdef PBG_DEBUG
static constexpr auto DBG_FN = "秋霜DBG.DAT";
#endif

// Validation helpers

static constexpr bool ValidGameLevel(GameLevel v) {
  return std::to_underlying(v) <= std::to_underlying(GameLevel::LUNATIC);
}
static constexpr bool ValidPlayerStock(uint8_t v) {
  return v <= (STOCK_PLAYER_MAX + 2);
}
static constexpr bool ValidBombStock(uint8_t v) {
  return v <= (STOCK_BOMB_MAX + 1);
}
static constexpr bool ValidPracticeMode(PracticeMode v) {
  return std::to_underlying(v) <= std::to_underlying(PracticeMode::INVINCIBLE);
}
static constexpr bool ValidFPSDivisor(uint8_t v) {
  return v <= FPS_DIVISOR_MAX;
}
static constexpr bool ValidScreenshotEffort(uint8_t v) {
  return v <= GRP_SCREENSHOT_EFFORT_MAX;
}
static constexpr bool ValidVolume(VOLUME v) { return v <= VOLUME_MAX; }
static constexpr bool ValidStageSelect(uint8_t v) { return v <= STAGE_MAX; }
static constexpr bool ValidWinMMPad(INPUT_PAD_BUTTON v) { return v <= 32; }

// TOML loading helper

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

static bool TOMLLoad(const char *fn) {
  auto *f = SDL_IOFromFile(fn, "rb");
  if (f == nullptr) {
    return false;
  }
  auto f_guard = make_guard(f, SDL_CloseIO);

  const auto size = SDL_GetIOSize(f);
  if (size <= 0) {
    return false;
  }

  std::string buf(static_cast<size_t>(size), '\0');
  if (SDL_ReadIO(f, buf.data(), size) != size) {
    return false;
  }

  toml::table tbl;
  try {
    tbl = toml::parse(buf);
  } catch (const toml::parse_error &) {
    return false;
  }

  // [difficulty]
  if (auto *sec = tbl["difficulty"].as_table()) {
    LoadToml(*sec, "game_level", ConfigDat.game_level, ValidGameLevel);
    LoadToml(*sec, "player_stock", ConfigDat.player_stock,
                 ValidPlayerStock);
    LoadToml(*sec, "bomb_stock", ConfigDat.bomb_stock, ValidBombStock);
    LoadToml(*sec, "practice_mode", ConfigDat.practice_mode,
                 ValidPracticeMode);
  }

  // [graphics]
  if (auto *sec = tbl["graphics"].as_table()) {
    LoadToml(*sec, "device_id", ConfigDat.device_id, [](auto) {
      return true;
    });
    LoadToml(*sec, "api", ConfigDat.graphics_api);
    LoadToml(*sec, "window_scale_4x", ConfigDat.window_scale_4x,
                 [](auto) { return true; });
    LoadToml(*sec, "window_left", ConfigDat.window_left);
    LoadToml(*sec, "window_top", ConfigDat.window_top);
    LoadToml(*sec, "fps_divisor", ConfigDat.fps_divisor, ValidFPSDivisor);
    LoadToml(*sec, "window_upper", ConfigDat.window_upper);
    LoadToml(*sec, "msg_disable", ConfigDat.msg_disable);
    LoadToml(*sec, "graphics_param_flags", ConfigDat.graphics_param_flags,
             [](GRAPHICS_PARAM_FLAGS f) {
               return (std::to_underlying(f) &
                       ~std::to_underlying(GRAPHICS_PARAM_FLAGS::MASK)) == 0;
             });
    LoadToml(*sec, "screenshot_effort", ConfigDat.screenshot_effort,
                 ValidScreenshotEffort);
  }

  // [sound]
  if (auto *sec = tbl["sound"].as_table()) {
    LoadToml(*sec, "bgm_enabled", ConfigDat.bgm_enabled);
    LoadToml(*sec, "se_enabled", ConfigDat.se_enabled);
    LoadToml(*sec, "bgm_volume_normalized", ConfigDat.bgm_vol_norm);
    LoadToml(*sec, "se_volume", ConfigDat.se_volume, ValidVolume);
    LoadToml(*sec, "bgm_volume", ConfigDat.bgm_volume, ValidVolume);
    LoadToml(*sec, "bgm_pack", ConfigDat.bgm_pack);
    LoadToml(*sec, "soundfont", ConfigDat.soundfont);
    bool midi_fix = false;
    LoadToml(*sec, "midi_fix_sysex_bugs", midi_fix);
    if (midi_fix) {
      ConfigDat.midi_flags |= MID_FLAGS::FIX_SYSEX_BUGS;
    } else {
      ConfigDat.midi_flags &= ~MID_FLAGS::FIX_SYSEX_BUGS;
    }
  }

  // [input]
  if (auto *sec = tbl["input"].as_table()) {
    LoadToml(*sec, "joypad_enabled", ConfigDat.joypad_enabled);
    LoadToml(*sec, "z_msg_skip_enabled", ConfigDat.z_msg_skip_enabled);
    LoadToml(*sec, "z_spd_down_enabled", ConfigDat.z_spd_down_enabled);
    LoadToml(*sec, "pad_tama", ConfigDat.pad_tama, ValidWinMMPad);
    LoadToml(*sec, "pad_bomb", ConfigDat.pad_bomb, ValidWinMMPad);
    LoadToml(*sec, "pad_shift", ConfigDat.pad_shift, ValidWinMMPad);
    LoadToml(*sec, "pad_cancel", ConfigDat.pad_cancel, ValidWinMMPad);
  }

  // [progress]
  if (auto *sec = tbl["progress"].as_table()) {
    LoadToml(*sec, "extra_stg_flags", ConfigDat.extra_stg_flags,
                 [](auto) { return true; });
    LoadToml(*sec, "stage_select", ConfigDat.stage_select,
                 ValidStageSelect);
  }

  return true;
}

static void TOMLSave(const char *fn) {
  toml::table tbl;

  // [difficulty]
  {
    toml::table sec;
    sec.emplace("game_level",
                std::to_underlying(ConfigDat.game_level));
    sec.emplace("player_stock", ConfigDat.player_stock);
    sec.emplace("bomb_stock", ConfigDat.bomb_stock);
    sec.emplace("practice_mode",
                std::to_underlying(ConfigDat.practice_mode));
    tbl.emplace("difficulty", std::move(sec));
  }

  // [graphics]
  {
    toml::table sec;
    sec.emplace("device_id", ConfigDat.device_id);
    sec.emplace("api", ConfigDat.graphics_api);
    sec.emplace("window_scale_4x", ConfigDat.window_scale_4x);
    sec.emplace("window_left", ConfigDat.window_left);
    sec.emplace("window_top", ConfigDat.window_top);
    sec.emplace("fps_divisor", ConfigDat.fps_divisor);
    sec.emplace("window_upper", ConfigDat.window_upper);
    sec.emplace("msg_disable", ConfigDat.msg_disable);
    sec.emplace("graphics_param_flags",
                std::to_underlying(ConfigDat.graphics_param_flags));
    sec.emplace("screenshot_effort", ConfigDat.screenshot_effort);
    tbl.emplace("graphics", std::move(sec));
  }

  // [sound]
  {
    toml::table sec;
    sec.emplace("bgm_enabled", ConfigDat.bgm_enabled);
    sec.emplace("se_enabled", ConfigDat.se_enabled);
    sec.emplace("bgm_volume_normalized", ConfigDat.bgm_vol_norm);
    sec.emplace("se_volume", ConfigDat.se_volume);
    sec.emplace("bgm_volume", ConfigDat.bgm_volume);
    sec.emplace("bgm_pack", ConfigDat.bgm_pack);
    sec.emplace("soundfont", ConfigDat.soundfont);
    sec.emplace("midi_fix_sysex_bugs",
                (ConfigDat.midi_flags & MID_FLAGS::FIX_SYSEX_BUGS) !=
                    MID_FLAGS::NONE);
    tbl.emplace("sound", std::move(sec));
  }

  // [input]
  {
    toml::table sec;
    sec.emplace("joypad_enabled", ConfigDat.joypad_enabled);
    sec.emplace("z_msg_skip_enabled", ConfigDat.z_msg_skip_enabled);
    sec.emplace("z_spd_down_enabled", ConfigDat.z_spd_down_enabled);
    sec.emplace("pad_tama", ConfigDat.pad_tama);
    sec.emplace("pad_bomb", ConfigDat.pad_bomb);
    sec.emplace("pad_shift", ConfigDat.pad_shift);
    sec.emplace("pad_cancel", ConfigDat.pad_cancel);
    tbl.emplace("input", std::move(sec));
  }

  // [progress]
  {
    toml::table sec;
    sec.emplace("extra_stg_flags", ConfigDat.extra_stg_flags);
    sec.emplace("stage_select", ConfigDat.stage_select);
    tbl.emplace("progress", std::move(sec));
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

// -------------------

GRAPHICS_PARAMS ConfigData::GraphicsParams() const {
  return {
      .flags = graphics_param_flags,
      .device_id = device_id,
#ifdef SUPPORT_GRP_API
      .api = GrpBackend_APIID(graphics_api),
#endif
      .window_scale_4x = window_scale_4x,
      .left = window_left,
      .top = window_top,
  };
}

void ConfigData::GraphicsParamsApply(const GRAPHICS_PARAMS &params) {
  graphics_param_flags = params.flags;
  device_id = params.device_id;
#ifdef SUPPORT_GRP_API
  graphics_api = GrpBackend_APIString(params.api);
#endif
  window_scale_4x = params.window_scale_4x;
  window_left = params.left;
  window_top = params.top;
}

uint8_t ConfigData::PackInputFlags() const {
  uint8_t v = 0;
  if (joypad_enabled)
    v |= 1;
  if (z_msg_skip_enabled)
    v |= 2;
  if (z_spd_down_enabled)
    v |= 4;
  return v;
}

void ConfigData::UnpackInputFlags(uint8_t v) {
  joypad_enabled = (v & 1) != 0;
  z_msg_skip_enabled = (v & 2) != 0;
  z_spd_down_enabled = (v & 4) != 0;
}

///// [Global variables] /////
ConfigData ConfigDat;
#ifdef PBG_DEBUG
DebugData DebugDat;
#endif

#ifdef PBG_DEBUG
static void DebugInit(void) {
  auto *f = SDL_IOFromFile(DBG_FN, "rb");
  if (!f) {
    return;
  }
  auto f_guard = make_guard(f, SDL_CloseIO);
  if (!SDL_MustReadIO(f, &DebugDat, sizeof(DebugDat))) {
    DebugDat.Hit = true;
    DebugDat.MsgDisplay = true;
    DebugDat.DemoSave = false;
    DebugDat.StgSelect = 1;
  }
}
#endif

// Initialize config contents
void ConfigData::Load() {
#ifdef PBG_DEBUG
  DebugInit();
#endif

  TOMLLoad(CFG_FN);
}

// Save config contents
void ConfigData::Save() {
  // Sync runtime audio state into config
  ConfigDat.bgm_enabled = BGM_Enabled();
  ConfigDat.bgm_vol_norm = BGM_GainApply();

  if (const auto maybe_topleft = WndBackend_Topleft()) {
    const auto &topleft = maybe_topleft.value();
    ConfigDat.window_left = topleft.first;
    ConfigDat.window_top = topleft.second;
  }

  TOMLSave(CFG_FN);

#ifdef PBG_DEBUG
  SDL_SaveFile(DBG_FN, &DebugDat, sizeof(DebugDat));
#endif
}
