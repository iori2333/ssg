///
/// Config - Config data
///

#pragma once

#include "game/input.h"
#include "game/midi.h"
#include "game/volume.h"
#include "level.h"
#include "platform/graphics_backend.h"

// Flags
constexpr uint8_t GRPF_PARAM_SHIFT = 3;
constexpr uint8_t GRPF_ORIG_MASK = 0x07;
constexpr uint8_t GRPF_PARAM_MASK =
    (std::to_underlying(GRAPHICS_PARAM_FLAGS::MASK) << GRPF_PARAM_SHIFT);

// Integer promotion...
constexpr uint8_t GRPF_MASK =
    static_cast<uint8_t>(~(GRPF_ORIG_MASK | GRPF_PARAM_MASK));

constexpr uint8_t GRPF_WINDOW_UPPER =
    0x02; // Show message window at top
constexpr uint8_t GRPF_MSG_DISABLE = 0x04; // Skip dialogue scenes

constexpr uint8_t SNDF_MASK = (~0x07);
constexpr uint8_t SNDF_BGM_ENABLE = 0x01;
constexpr uint8_t SNDF_SE_ENABLE = 0x02;

// Normalizing the volume should be the default, so we need to store the
// negation for backwards compatibility.
constexpr uint8_t SNDF_BGM_NOT_VOL_NORM = 0x04;

constexpr uint8_t INPF_MASK = (~0x07);
constexpr uint8_t INPF_JOYPAD_ENABLE = 0x01;    // Joypad enabled
constexpr uint8_t INPF_Z_MSKIP_ENABLE = 0x02;   // Z key advances message
constexpr uint8_t INPF_Z_SPDDOWN_ENABLE = 0x04; // Hold for shift movement

constexpr uint8_t DBGF_MASK = (~0x03);
constexpr uint8_t DBGF_DISPLAYINFO = 0x01; // Show debug info
constexpr uint8_t DBGF_HIT_ON = 0x02;      // Hit detection enabled

// Limits
constexpr const auto STOCK_PLAYER_MAX = 6;
constexpr const auto STOCK_BOMB_MAX = 6;
constexpr const auto FPS_DIVISOR_MAX = 3;
constexpr const auto STAGE_MAX = 6; // Number of stages

// Practice mode
constexpr const auto PRACTICE_OFF = 0;
constexpr const auto PRACTICE_AUTOBOMB = 1;
constexpr const auto PRACTICE_INVINCIBLE = 2;

// AutoPlay
constexpr const auto AUTOPLAY_OFF = 0;
constexpr const auto AUTOPLAY_ON = 1;

bool ValidateAlways(auto v) { return true; };
template <typename T, T Max> constexpr bool ValidateBelow(T v) {
  return (v <= Max);
}
template <uint8_t Mask> constexpr bool ValidateMask(uint8_t v) {
  return ((v & Mask) == 0);
}
template <ENUMFLAGS Flag> constexpr bool ValidateFlag(Flag v) {
  return ((std::to_underlying(v) & ~std::to_underlying(Flag::MASK)) == 0);
}

// Option class, with optional validation of the value against a supported
// static range.
template <typename T> struct CONFIG_OPTION_VALUE {
  T v;

  // ([loaded] == std::nullopt): We loaded a config file from an earlier
  // build, where this value was not present yet. It was initialized to its
  // default value.
  // ([loaded] == [v]): Option was present in a config file, and its value
  // passed validation.
  // ([loaded] != [v]): The config file contained an invalid value, and was
  // reset to its default.
  std::optional<T> loaded = std::nullopt;

  bool (*Validate)(T);

  constexpr CONFIG_OPTION_VALUE(T def = T{},
                                bool (*validate)(T) = ValidateAlways) noexcept
      : v(def), Validate(validate) {}
};

// Option storage struct
struct CONFIG_DATA {
  template <typename T> using OPTION = CONFIG_OPTION_VALUE<T>;

  template <class T, T V> static constexpr auto Below = &ValidateBelow<T, V>;
  template <uint8_t V> static constexpr auto U8Below = Below<uint8_t, V>;
  template <uint8_t V> static constexpr auto Mask = &ValidateMask<V>;
  template <ENUMFLAGS V> static constexpr auto Flag = &ValidateFlag<V>;
  static constexpr auto ValidateVolume = &ValidateBelow<VOLUME, VOLUME_MAX>;

  // 32 is the WinMM joy button limit
  static constexpr auto ValidateWinMMPad = Below<INPUT_PAD_BUTTON, 32>;

  // Difficulty settings

  // Difficulty
  OPTION<uint8_t> GameLevel = {GAME_NORMAL, U8Below<GAME_LUNATIC>};

  // Initial maids count?
  // Quirk: Off by 2?
  OPTION<uint8_t> PlayerStock = {2, U8Below<(STOCK_PLAYER_MAX + 2)>};

  // Initial bombs count
  // Quirk: Off by 1?
  OPTION<uint8_t> BombStock = {2, U8Below<(STOCK_BOMB_MAX + 1)>};

  // Practice mode: OFF/AUTOBOMB/INVINCIBLE
  OPTION<uint8_t> PracticeMode = {PRACTICE_OFF, U8Below<PRACTICE_INVINCIBLE>};

  // AutoPlay: OFF/ON
  OPTION<uint8_t> AutoPlay = {AUTOPLAY_OFF, U8Below<AUTOPLAY_ON>};

  // Graphics settings
  OPTION<uint8_t> DeviceID = {0}; // Device index
  std::string GraphicsAPI;
  OPTION<uint8_t> WindowScale4x = {0};
  OPTION<int16_t> WindowLeft = {GRAPHICS_TOPLEFT_UNDEFINED};
  OPTION<int16_t> WindowTop = {GRAPHICS_TOPLEFT_UNDEFINED};
  OPTION<BITDEPTH> BitDepth; // Bit depth

  // Target frame rate = 60 / [FPSDivisor]. 0 disables any frame rate
  // limitation.
  OPTION<uint8_t> FPSDivisor = {1, U8Below<FPS_DIVISOR_MAX>};

  // Graphics flags
  OPTION<uint8_t> GraphFlags = {0, Mask<GRPF_MASK>};
  OPTION<uint8_t> ScreenshotEffort = {0, U8Below<GRP_SCREENSHOT_EFFORT_MAX>};

  // Sound/BGM settings
  OPTION<uint8_t> SoundFlags = {(SNDF_SE_ENABLE | SNDF_BGM_ENABLE),
                                Mask<SNDF_MASK>};
  OPTION<MID_FLAGS> MidFlags = {MID_FLAGS::FIX_SYSEX_BUGS, Flag<MID_FLAGS>};
  OPTION<VOLUME> SEVolume = {((VOLUME_MAX * 4) / 10), ValidateVolume};
  OPTION<VOLUME> BGMVolume = {((VOLUME_MAX * 4) / 10), ValidateVolume};
  std::string BGMPack;

  // Input flags
  OPTION<uint8_t> InputFlags = {INPF_Z_MSKIP_ENABLE, Mask<INPF_MASK>};

  // Debug flags
  OPTION<uint8_t> DebugFlags = {0, Mask<DBGF_MASK>};

  OPTION<INPUT_PAD_BUTTON> PadTama = {1, ValidateWinMMPad};
  OPTION<INPUT_PAD_BUTTON> PadBomb = {2, ValidateWinMMPad};
  OPTION<INPUT_PAD_BUTTON> PadShift = {0, ValidateWinMMPad};
  OPTION<INPUT_PAD_BUTTON> PadCancel = {0, ValidateWinMMPad};

  // Extra stage progress flags
  OPTION<uint8_t> ExtraStgFlags = {0};

  // Placed here intentionally (outside checksum range)
  OPTION<uint8_t> StageSelect = {0, U8Below<STAGE_MAX>};

  [[nodiscard]] GRAPHICS_PARAMS GraphicsParams() const;
  void GraphicsParamsApply(const GRAPHICS_PARAMS &params);
};

// Active configuration
extern CONFIG_DATA ConfigDat;

#ifdef PBG_DEBUG
// Debug info management struct
struct DebugData {
  int32_t MsgDisplay; // Show debug info
  int32_t Hit;        // Hit detection on/off
  int32_t DemoSave;   // Save demo replay

  uint8_t StgSelect; // Stage select (starting stage)
};

extern DebugData DebugDat;
#endif

// Functions

void ConfigLoad();

void ConfigSave(); // Save config to file
