/*
 *   Config data
 *
 */

#include <SDL3/SDL_iostream.h>

#include "CONFIG.H"
#include "game/bgm.h"
#include "game/defer.h"
#include "game/endian.h"
#include "platform/file.h"
#include "platform/window_backend.h"

///// Constants /////
constexpr auto DBG_FN = u8"秋霜DBG.DAT";

// Data types
// ----------

template <typename T>
using DISK =
    std::conditional_t<std::is_fundamental_v<T>, ENDIAN_SELECT_BIG<T>, T>;

template <typename T>
bool OptionRead(CONFIG_OPTION_VALUE<T> &opt, SDL_IOStream &f) {
  DISK<T> d_loaded;
  if (!SDL_MustReadIO(&f, &d_loaded, sizeof(d_loaded))) {
    return false;
  }
  const T loaded = d_loaded;
  opt.loaded = loaded;
  if (opt.Validate(loaded)) {
    opt.v = loaded;
  }
  return true;
}

template <typename T>
bool OptionWrite(SDL_IOStream &f, const CONFIG_OPTION_VALUE<T> &opt) {
  const DISK<T> d_v = opt.v;
  return SDL_MustWriteIO(&f, &d_v, sizeof(T));
}

bool OptionRead(std::u8string &opt, SDL_IOStream &f) {
  U32BE d_len;
  if (!SDL_MustReadIO(&f, &d_len, sizeof(d_len))) {
    return false;
  }
  const uint32_t len = d_len;
  opt.resize_and_overwrite(
      len, [&f](auto buf, size_t n) { return SDL_ReadIO(&f, buf, n); });
  return ((opt.size() * sizeof(std::u8string::value_type)) == len);
}

bool OptionWrite(SDL_IOStream &f, const std::u8string &opt) {
  if (opt.size() > (std::numeric_limits<uint32_t>::max)()) {
    return false;
  }
  const auto d_size = static_cast<U32BE>(opt.size());
  if (!SDL_MustWriteIO(&f, &d_size, sizeof(d_size))) {
    return false;
  }
  const auto str_size = (opt.size() * sizeof(std::u8string::value_type));
  return SDL_MustWriteIO(&f, opt.data(), str_size);
}

template <class... Options>
bool OptionRead(const std::tuple<Options &...> &opts, SDL_IOStream &f) {
  return std::apply([&](auto &...opt) { return (... && OptionRead(opt, f)); },
                    opts);
}

template <class... Options>
bool OptionWrite(SDL_IOStream &f, const std::tuple<Options &...> &opts) {
  return std::apply(
      [&](const auto &...opt) { return (... && OptionWrite(f, opt)); }, opts);
}
// ----------

// On-disk config file
// -------------------

static constexpr auto CFG_FN = u8"SSG.CFG";

static constexpr auto CFG_OPTIONS =
    std::tie(ConfigDat.GameLevel, ConfigDat.PlayerStock, ConfigDat.BombStock,
             ConfigDat.DeviceID, ConfigDat.BitDepth, ConfigDat.FPSDivisor,
             ConfigDat.GraphFlags, ConfigDat.SoundFlags, ConfigDat.InputFlags,
             ConfigDat.DebugFlags, ConfigDat.PadTama, ConfigDat.PadBomb,
             ConfigDat.PadShift, ConfigDat.PadCancel, ConfigDat.ExtraStgFlags,
             ConfigDat.StageSelect, ConfigDat.SEVolume, ConfigDat.BGMVolume,
             ConfigDat.BGMPack, ConfigDat.MidFlags, ConfigDat.GraphicsAPI,
             ConfigDat.WindowScale4x, ConfigDat.WindowLeft,
             ConfigDat.WindowTop, ConfigDat.ScreenshotEffort,
             ConfigDat.PracticeMode);

static bool ConfigFileLoad() {
  SDL_IOStream *f = SDL_IOFromFile(CFG_FN, "rb");
  if (!f) {
    return false;
  }
  defer(SDL_CloseIO(f));
  return OptionRead(CFG_OPTIONS, *f);
}

static void ConfigFileSave() {
  SDL_IOStream *f = SDL_IOFromFile(CFG_FN, "wb");
  if (!f) {
    return;
  }
  defer(SDL_CloseIO(f));
  OptionWrite(*f, CFG_OPTIONS);
}
// -------------------

GRAPHICS_PARAMS CONFIG_DATA::GraphicsParams(void) const {
  const auto flags_shifted = (GraphFlags.v >> GRPF_PARAM_SHIFT);
  return {
      .flags = static_cast<GRAPHICS_PARAM_FLAGS>(flags_shifted),
      .device_id = DeviceID.v,
#ifdef SUPPORT_GRP_API
      .api = GrpBackend_APIID(GraphicsAPI),
#endif
      .window_scale_4x = WindowScale4x.v,
      .left = WindowLeft.v,
      .top = WindowTop.v,
      .bitdepth = BitDepth.v,
  };
}

void CONFIG_DATA::GraphicsParamsApply(const GRAPHICS_PARAMS &params) {
  GraphFlags.v &= ~GRPF_PARAM_MASK;
  GraphFlags.v |= (std::to_underlying(params.flags) << GRPF_PARAM_SHIFT);
  DeviceID.v = params.device_id;
#ifdef SUPPORT_GRP_API
  GraphicsAPI = GrpBackend_APIString(params.api);
#endif
  WindowScale4x.v = params.window_scale_4x;
  WindowLeft.v = params.left;
  WindowTop.v = params.top;
  BitDepth.v = params.bitdepth;
}

///// [グローバル変数] /////
CONFIG_DATA ConfigDat;
#ifdef PBG_DEBUG
DEBUG_DATA DebugDat;
#endif

#ifdef PBG_DEBUG
static void DebugInit(void) {
  auto *f = SDL_IOFromFile(DBG_FN, "rb");
  if (!f) {
    return;
  }
  if (!SDL_MustReadIO(f, &DebugDat, sizeof(DebugDat))) {
    DebugDat.Hit = true;
    DebugDat.MsgDisplay = true;
    DebugDat.DemoSave = false;
    DebugDat.StgSelect = 1;
  }
  SDL_CloseIO(f);
}
#endif

// コンフィグの内容を初期化する //
extern void ConfigLoad() {
#ifdef PBG_DEBUG
  DebugInit();
#endif

  ConfigFileLoad();
}

// コンフィグの内容を保存する //
extern void ConfigSave(void) {
  // Sync runtime audio state into config
  ConfigDat.SoundFlags.v &= SNDF_SE_ENABLE;
  ConfigDat.SoundFlags.v |= (BGM_Enabled() * SNDF_BGM_ENABLE);
  ConfigDat.SoundFlags.v |= (!BGM_GainApply() * SNDF_BGM_NOT_VOL_NORM);

  if (const auto maybe_topleft = WndBackend_Topleft()) {
    const auto &topleft = maybe_topleft.value();
    ConfigDat.WindowLeft.v = topleft.first;
    ConfigDat.WindowTop.v = topleft.second;
  }

  ConfigFileSave();

#ifdef PBG_DEBUG
  SDL_SaveFile(DBG_FN, &DebugDat, sizeof(DebugDat));
#endif
}
