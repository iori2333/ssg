///
/// Panels - Menu panel class implementation
///

#include "panels.h"

#include "config.h"
#include "demo_play.h"
#include "entry.h"
#include "game/bgm.h"
#include "game/midi.h"
#include "game/snd.h"
#include "game_main.h"
#include "gameflow/gameflow_manager.h"
#include "level.h"
#include "loader.h"
#include "music.h"
#include "platform/graphics_backend.h"
#include "platform/input.h"
#include "platform/midi_backend.h"
#include "ui_manager.h"
#include "window_sys.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>
#include <algorithm>
#include <chrono>
#include <numeric>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr void RingStep(uint8_t &var, int_fast8_t delta, uint8_t min,
                               uint8_t max) {
  var = ((delta < 0) ? ((var <= min) ? max : (var - 1))
                     : ((var == max) ? min : (var + 1)));
}

static constexpr const char *CHOICE_OFF_ON[2] = {"[O F F]", "[ O N ]"};
static constexpr const char *CHOICE_OFF_ON_NARROW[2] = {"[  ]", "[●]"};
static constexpr const char *CHOICE_USE[2] = {" 使用する ", "使用しない"};

constexpr auto HELP_SUBMENU_EXIT = "一つ前のメニューにもどります";
constexpr auto HELP_API_DEFAULT = "Let the backend choose a graphics API";
constexpr auto HELP_API_SPECIFIC = "Select to override default API selection";

// Shared items
static MenuItem SubmenuExitItem = {"Exit", HELP_SUBMENU_EXIT, CWinExitFn};
static MenuItem HRuleItem = {"-------------------"};

// ---------------------------------------------------------------------------
// DifficultyPanel
// ---------------------------------------------------------------------------

DifficultyPanel::DifficultyPanel() {
  items_.reserve(13);
  items_.emplace_back(titles_[0].Lit(), "残り人数?を設定します", FnPlayerStock);
  items_.emplace_back(titles_[1].Lit(), "ボムの数を設定します", FnBombStock);
  items_.emplace_back(titles_[2].Lit(), "難易度を設定します", FnDifficulty);
  items_.emplace_back(titles_[3].Lit(), "练习模式を設定します", FnPracticeMode);
#ifdef PBG_DEBUG
  items_.emplace_back(HRuleItem);
  items_.emplace_back(titles_[4].Lit(), "[DebugMode] 画面に情報を表示するか",
                      FnMsgDisplay);
  items_.emplace_back(titles_[5].Lit(), "[DebugMode] ステージセレクト",
                      FnStgSelect);
  items_.emplace_back(titles_[6].Lit(), "[DebugMode] 当たり判定", FnHit);
  items_.emplace_back(titles_[7].Lit(), "[DebugMode] デモプレイセーブ", FnDemo);
#endif
  items_.emplace_back(SubmenuExitItem);
  menu_ = MenuDef(std::span(items_),
                  [this](MenuController &c, bool t) { Refresh(c, t); });
}

void DifficultyPanel::FnPlayerStock(MenuController &, int_fast8_t delta) {
  RingStep(ConfigDat.PlayerStock.v, delta, 0, STOCK_PLAYER_MAX);
}

void DifficultyPanel::FnBombStock(MenuController &, int_fast8_t delta) {
  RingStep(ConfigDat.BombStock.v, delta, 0, STOCK_BOMB_MAX);
}

void DifficultyPanel::FnDifficulty(MenuController &, int_fast8_t delta) {
  RingStep(ConfigDat.GameLevel.v, delta, GAME_EASY, GAME_LUNATIC);
}

void DifficultyPanel::FnPracticeMode(MenuController &, int_fast8_t delta) {
  RingStep(ConfigDat.PracticeMode.v, delta, PRACTICE_OFF, PRACTICE_INVINCIBLE);
}

#ifdef PBG_DEBUG
void DifficultyPanel::FnMsgDisplay(MenuController &, int_fast8_t) {
  DebugDat.MsgDisplay = !DebugDat.MsgDisplay;
}
void DifficultyPanel::FnStgSelect(MenuController &, int_fast8_t delta) {
  RingStep(DebugDat.StgSelect, delta, 1, STAGE_MAX);
}
void DifficultyPanel::FnHit(MenuController &, int_fast8_t) {
  DebugDat.Hit = !DebugDat.Hit;
}
void DifficultyPanel::FnDemo(MenuController &, int_fast8_t) {
  DebugDat.DemoSave = !DebugDat.DemoSave;
}
#endif

void DifficultyPanel::Refresh(MenuController &, bool) {
  static constexpr const char *const dif[4] = {" Easy  ", " Normal", " Hard  ",
                                               "Lunatic"};
  static constexpr const char *const practice[3] = {" Off ", "AutoB", "Invin"};

  titles_[0].Format("PlayerStock [ {} ]", ConfigDat.PlayerStock.v + 1);
  titles_[1].Format("BombStock   [ {} ]", ConfigDat.BombStock.v);
  titles_[2].Format("Difficulty[{}]", dif[ConfigDat.GameLevel.v]);
  titles_[3].Format("PracticeMode[{}]", practice[ConfigDat.PracticeMode.v]);

#ifdef PBG_DEBUG
  titles_[4].Format("DebugOut  {}", CHOICE_OFF_ON[DebugDat.MsgDisplay]);
  titles_[5].Format("StgSelect [  {}  ]", DebugDat.StgSelect);
  titles_[6].Format("Hit       {}", CHOICE_OFF_ON[DebugDat.Hit]);
  titles_[7].Format("DemoSave  {}", CHOICE_OFF_ON[DebugDat.DemoSave]);
#endif

  for (size_t i = 0; i < 8; i++) {
    items_[i].Title = titles_[i].Lit();
  }
}

// ---------------------------------------------------------------------------
// ScreenshotPanel
// ---------------------------------------------------------------------------

ScreenshotPanel::ScreenshotPanel() {
  items_.reserve(2 + GRP_SCREENSHOT_EFFORT_COUNT + 2);
  items_.emplace_back(title_format_.Lit(), "", FnFormat);
  items_.emplace_back("--- Performance ---");
  for (auto &t : title_perf_) {
    items_.emplace_back(t.Lit(), help_perf_.c_str(), FnFormat);
  }
  items_.emplace_back(HRuleItem);
  items_.emplace_back(SubmenuExitItem);
  menu_ = MenuDef(std::span(items_),
                  [this](MenuController &c, bool t) { Refresh(c, t); });
}

void ScreenshotPanel::FnFormat(MenuController &, int_fast8_t delta) {
  RingStep(ConfigDat.ScreenshotEffort.v, delta, 0, GRP_SCREENSHOT_EFFORT_MAX);
}

void ScreenshotPanel::RefreshActive(MenuController &ctrl) {
  Refresh(ctrl, false);
}

void ScreenshotPanel::Refresh(MenuController &ctrl, bool) {
  const auto effort = ConfigDat.ScreenshotEffort.v;
  enum class ALIGN { LEFT, CENTER };
  const auto format_for = [](uint8_t e, ALIGN align) -> std::string {
    if (e == 0) {
      return (align == ALIGN::LEFT) ? "BMP    " : "  BMP  ";
    }
    return std::format("WebP z{}", e - 1);
  };

  const auto split_into_fraction = [](auto v) {
    return std::pair{static_cast<unsigned>(v / 1000),
                     static_cast<unsigned>((v % 1000) / 10)};
  };

  title_format_.Format("Format    [{}]", format_for(effort, ALIGN::CENTER));
  items_[0].Title = title_format_.Lit();
  if (effort == 0) {
    items_[0].Help = "Saving as uncompressed .BMP";
  } else {
    items_[0].Help = "Lossless compression (higher = slower)";
  }

  for (const auto i : std::views::iota(0U, GRP_SCREENSHOT_EFFORT_COUNT)) {
    auto &item = items_[2 + i];
    const auto hovered = (ctrl.CurrentSelection() == (2 + i));
    const auto fmt = format_for(i, ALIGN::LEFT);
    const auto time = Grp_ScreenshotTimes[i];
    EnumFlagSet(item.Flags, MenuFlags::HIGHLIGHT,
                static_cast<std::underlying_type_t<MenuFlags>>(i == effort));

    if (time < 0s) {
      title_perf_[i].Format("{}[  FAILED  ]", fmt);
      if (hovered) {
        help_perf_.Set("https://github.com/nmlgc/ssg/issues/23");
      }
    } else if (time == 0s) {
      title_perf_[i].Format("{}[    ？    ]", fmt);
      if (hovered) {
        help_perf_.Set("Not yet measured");
      }
    } else {
      const auto [t_int, t_frac] = split_into_fraction(
          std::chrono::duration_cast<decltype(0us)>(time).count() + 5);
      title_perf_[i].Format("{}[{:5}.{:02}ms]", fmt, t_int, t_frac);
      if (hovered) {
        constexpr auto target_ms = decltype(0ms)(FRAME_TIME_TARGET);
        if (time > (target_ms * ConfigDat.FPSDivisor.v)) {
          const auto [fps_int, fps_frac] = split_into_fraction(
              std::chrono::milliseconds(1000).count() / time.count());
          help_perf_.Format("Frame rate will drop to ~{}.{:02} FPS", fps_int,
                            fps_frac);
        } else {
          static constexpr const char *FPS[3] = {"62.5", "30", "20"};
          assert(ConfigDat.FPSDivisor.v > 0);
          assert((ConfigDat.FPSDivisor.v - 1) < std::size(FPS));
          help_perf_.Format("Frame rate will stay at {} FPS",
                            FPS[ConfigDat.FPSDivisor.v - 1]);
        }
      }
    }
    item.Title = title_perf_[i].Lit();
    item.Help = help_perf_.c_str();
  }
}

// ---------------------------------------------------------------------------
// ApiPanel
// ---------------------------------------------------------------------------

ApiPanel::ApiPanel()
    : item_def_(title_def_.Lit(), HELP_API_DEFAULT, FnDef),
      menu_(std::span<MenuItem, 0>(),
            [this](MenuController &c, bool t) { Refresh(c, t); }) {}

void ApiPanel::FnDef(MenuController &, int_fast8_t) {
  XGrpTry([](auto &params) { params.api = -1; });
}

void ApiPanel::FnOverride(MenuController &ctrl, int_fast8_t) {
  XGrpTry([&](auto &params) { params.api = (ctrl.CurrentSelection() - 1); });
}

void ApiPanel::Init() {
  const auto grp_api_count = GrpBackend_APICount();
  assert(grp_api_count <= 8);

  auto *menu_p = menu_.ItemPtr;
  *(menu_p++) = &item_def_;
  for (const auto i : std::views::iota(0, grp_api_count)) {
    const auto driver_str = GrpBackend_APIString(i);
    const auto label = GrpBackend_APILabel(driver_str);
    assert(!label.empty());
    items_[i] = MenuItem(reinterpret_cast<const char *>(label.data()),
                         HELP_API_SPECIFIC, FnOverride);
    *(menu_p++) = &items_[i];
  }
  *(menu_p++) = &SubmenuExitItem;
  menu_.NumItems = static_cast<uint8_t>(std::distance(menu_.ItemPtr, menu_p));
}

void ApiPanel::Refresh(MenuController &, bool) {
  const bool is_def_api = ConfigDat.GraphicsAPI.empty();
  std::string_view api_active = GrpBackend_APILabel(GrpBackend_APIString());

  item_def_.SetActive(!is_def_api);
  for (auto &api : items_ | std::views::take(GrpBackend_APICount())) {
    const auto is_selected = strcmp(api_active.data(), api.Title) == 0;
    EnumFlagSet(api.Flags, MenuFlags::HIGHLIGHT,
                static_cast<std::underlying_type_t<MenuFlags>>(is_selected));
  }

  title_def_.Format("UseDefault  {}", CHOICE_OFF_ON[is_def_api]);
  item_def_.Title = title_def_.Lit();
}

// ---------------------------------------------------------------------------
// GraphicsPanel
// ---------------------------------------------------------------------------

GraphicsPanel::GraphicsPanel() {
  item_storage_.reserve(12);

#ifdef SUPPORT_GRP_WINDOWED
  item_storage_.emplace_back(
      title_disp_.Lit(), "Switch between window and fullscreen modes", FnDisp);
  item_ptrs_.push_back(&item_storage_.back());
  item_storage_.emplace_back(title_fs_mode_.Lit(), help_fs_mode_.c_str(),
                             FnFSMode);
  item_ptrs_.push_back(&item_storage_.back());
#endif
#ifdef SUPPORT_GRP_SCALING
  item_storage_.emplace_back(title_scale_.Lit(), help_scale_.c_str(), FnScale);
  item_ptrs_.push_back(&item_storage_.back());
  item_storage_.emplace_back(title_sc_mode_.Lit(), help_sc_mode_.c_str(),
                             FnScMode);
  item_ptrs_.push_back(&item_storage_.back());
#endif
  item_storage_.emplace_back(title_skip_.Lit(), "描画スキップの設定です",
                             FnSkip);
  item_ptrs_.push_back(&item_storage_.back());
  item_storage_.emplace_back("Screenshots", "Customize the screenshot format",
                             screenshot_.Menu());
  item_ptrs_.push_back(&item_storage_.back());
#ifdef SUPPORT_GRP_API
  item_api_ = MenuItem{"API", "Select rendering API", api_.Menu()};
  item_ptrs_.push_back(&item_api_);
#endif
  item_ptrs_.push_back(&HRuleItem);
  item_storage_.emplace_back(title_msg_.Lit(), "ウィンドウの表示位置を決めます",
                             FnWinLocate);
  item_ptrs_.push_back(&item_storage_.back());
  item_ptrs_.push_back(&SubmenuExitItem);

  menu_ = MenuDef([this](MenuController &c, bool t) { Refresh(c, t); },
                  std::span<MenuItem *const>(item_ptrs_));
}

void GraphicsPanel::Init() {
#ifdef SUPPORT_GRP_API
  if (GrpBackend_APICount() >= 2) {
    api_.Init();
  } else {
    item_api_.Flags |= MenuFlags::DISABLED;
  }
#endif
}

void GraphicsPanel::FnDisp(MenuController &, int_fast8_t) {
  XGrpTryCycleDisp();
}

void GraphicsPanel::FnFSMode(MenuController &, int_fast8_t) {
  XGrpTry([](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE;
  });
}

void GraphicsPanel::FnScale(MenuController &, int_fast8_t delta) {
  XGrpTryCycleScale(delta, true);
}

void GraphicsPanel::FnScMode(MenuController &, int_fast8_t) {
  XGrpTryCycleScMode();
}

void GraphicsPanel::FnSkip(MenuController &, int_fast8_t delta) {
  RingStep(ConfigDat.FPSDivisor.v, delta, 0, FPS_DIVISOR_MAX);
  Grp_FPSDivisor = ConfigDat.FPSDivisor.v;
}

void GraphicsPanel::FnWinLocate(MenuController &, int_fast8_t delta) {
  static constexpr uint8_t flags[3] = {0, GRPF_WINDOW_UPPER, GRPF_MSG_DISABLE};
  const auto *it = std::ranges::find_if(flags, [](auto f) {
    return ((ConfigDat.GraphFlags.v & GRPF_ORIG_MASK) == f);
  });
  const auto i = ((it != std::end(flags)) ? std::distance(flags, it) : 0);

  ConfigDat.GraphFlags.v &= ~GRPF_ORIG_MASK;
  if (delta < 0) {
    ConfigDat.GraphFlags.v |= flags[(i + 2) % std::size(flags)];
  } else {
    ConfigDat.GraphFlags.v |= flags[(i + 1) % std::size(flags)];
  }
}

void GraphicsPanel::Refresh(MenuController &, bool) {
  const auto params = ConfigDat.GraphicsParams();

  static constexpr auto aspect = (GRP_RES / std::gcd(GRP_RES.w, GRP_RES.h));
  static constexpr const char *DISPLAY_MODES[] = {"  Window  ", "Fullscreen"};
  static constexpr const char *FULLSCREEN_MODES[] = {"Borderless",
                                                     "Exclusive "};
  static constexpr std::tuple<const char *, bool> SCALE_MODES[] = {
      {"FrameBuf", false}, {"Geometry", false}, {"--------", true}};
  static constexpr const char *const UorD[3] = {"上のほう", "下のほう",
                                                "描画せず"};
  static constexpr const char *const FRate[4] = {"おまけ", "60Fps", "30Fps",
                                                 "20Fps"};

  const auto u_or_d =
      (((ConfigDat.GraphFlags.v & GRPF_MSG_DISABLE) != 0)
           ? 2
           : (((ConfigDat.GraphFlags.v & GRPF_WINDOW_UPPER) != 0) ? 0 : 1));
  const auto dev = GrpBackend_DeviceLabel(ConfigDat.DeviceID.v);
  const auto fs = params.FullscreenFlags();
  const auto in_borderless_fullscreen = (fs.fullscreen && !fs.exclusive);

  title_device_.Format("Device   [{:.7}]", dev);
#ifdef SUPPORT_GRP_WINDOWED
  title_disp_.Format("Display[{}]", DISPLAY_MODES[fs.fullscreen]);
  title_fs_mode_.Format("FullScr[{}]", FULLSCREEN_MODES[fs.exclusive]);
#endif
#ifdef SUPPORT_GRP_SCALING
  const auto scale_4x = params.Scale4x();
  const auto scale_res = params.ScaledRes();

  if (in_borderless_fullscreen) {
    switch (fs.fit) {
    case GRAPHICS_FULLSCREEN_FIT::INTEGER:
      title_scale_.Set("FullScrFit[Integer]");
      break;
    case GRAPHICS_FULLSCREEN_FIT::ASPECT:
      title_scale_.Format("FullScrFit[  {}:{}  ]", aspect.w, aspect.h);
      break;
    case GRAPHICS_FULLSCREEN_FIT::STRETCH:
      title_scale_.Set("FullScrFit[Stretch]");
      break;
    }
  } else if (scale_4x != 0U) {
    const auto sv1 = (scale_4x / 4U);
    const auto sv2 = ((scale_4x % 4U) * 25U);
    title_scale_.Format("ScaleFact[{:3}.{:02}x ]", sv1, sv2);
  } else {
    title_scale_.Set("ScaleFact[ Screen ]");
  }

  const auto [sc_mode_label, sc_mode_disabled] =
      ((scale_4x == 4)          ? SCALE_MODES[2]
       : params.ScaleGeometry() ? SCALE_MODES[1]
                                : SCALE_MODES[0]);
  title_sc_mode_.Format("ScaleMode[{}]", sc_mode_label);

#ifdef SUPPORT_GRP_WINDOWED
  constexpr size_t SCALE_IDX = 2;
  constexpr size_t SC_MODE_IDX = 3;
#else
  constexpr size_t SCALE_IDX = 0;
  constexpr size_t SC_MODE_IDX = 1;
#endif
  item_ptrs_[SCALE_IDX]->SetActive(!(fs.fullscreen && fs.exclusive));
  EnumFlagSet(item_ptrs_[SC_MODE_IDX]->Flags, MenuFlags::DISABLED,
              static_cast<std::underlying_type_t<MenuFlags>>(sc_mode_disabled));
#endif
  title_skip_.Format("FrameRate[ {} ]", FRate[ConfigDat.FPSDivisor.v]);
  title_msg_.Format("MsgWindow[{}]", UorD[u_or_d]);

  // Help strings
#ifdef SUPPORT_GRP_WINDOWED
  if (fs.exclusive) {
    help_fs_mode_.Format("Fullscreen changes resolution to {}x{}", GRP_RES.w,
                         GRP_RES.h);
  } else {
    help_fs_mode_.Set("Fullscreen uses a display-sized window");
  }
#endif
#ifdef SUPPORT_GRP_SCALING
  if (in_borderless_fullscreen) {
    if (fs.fit == GRAPHICS_FULLSCREEN_FIT::INTEGER) {
      help_scale_.Set(std::format("Use largest integer {}:{} resolution",
                                  aspect.w, aspect.h));
    } else if (fs.fit == GRAPHICS_FULLSCREEN_FIT::ASPECT) {
      help_scale_.Set(std::format("Use largest fractional {}:{} resolution",
                                  aspect.w, aspect.h));
    } else {
      help_scale_.Set("Use aspect ratio of display");
    }
  } else if (scale_4x == 0) {
    help_scale_.Set("Game scales to fit the display");
  } else if (scale_4x == 4) {
    help_scale_.Format("Window size is {}\u00d7{}, not scaling", GRP_RES.w,
                       GRP_RES.h);
  } else {
    help_scale_.Format("Game is scaled to {}\u00d7{}", scale_res.w,
                       scale_res.h);
  }

  if (params.ScaleGeometry()) {
    help_sc_mode_.Format("Render at {}\u00d7{}, scale each draw call",
                         scale_res.w, scale_res.h);
  } else {
    help_sc_mode_.Format("Render at {}\u00d7{}, then scale framebuffer",
                         GRP_RES.w, GRP_RES.h);
  }
#endif

  // Re-point item titles
  size_t idx = 0;
#ifdef SUPPORT_GRP_WINDOWED
  item_ptrs_[idx++]->Title = title_disp_.Lit();
  item_ptrs_[idx]->Title = title_fs_mode_.Lit();
  item_ptrs_[idx]->Help = help_fs_mode_.c_str();
  idx++;
#endif
#ifdef SUPPORT_GRP_SCALING
  item_ptrs_[idx]->Title = title_scale_.Lit();
  item_ptrs_[idx]->Help = help_scale_.c_str();
  idx++;
  item_ptrs_[idx]->Title = title_sc_mode_.Lit();
  item_ptrs_[idx]->Help = help_sc_mode_.c_str();
  idx++;
#endif
  item_ptrs_[idx++]->Title = title_skip_.Lit();
  // ItemScreenshot (static title, no update)
  idx++;
  // ItemAPI (static title, no update)
  idx++;
  // HRule (static)
  idx++;
  item_ptrs_[idx++]->Title = title_msg_.Lit();
}

// ---------------------------------------------------------------------------
// MidiPanel
// ---------------------------------------------------------------------------

MidiPanel::MidiPanel() {
  items_.reserve(3);
  items_.emplace_back(title_port_.Lit(), "MIDI Port (保存はされません)", FnDev);
  items_.emplace_back(title_fixes_.Lit(),
                      "Retain SC-88Pro echo on other Roland synths", FnFixes);
  items_.emplace_back(SubmenuExitItem);
  menu_ = MenuDef(std::span(items_),
                  [this](MenuController &c, bool t) { Refresh(c, t); });
}

void MidiPanel::FnDev(MenuController &, int_fast8_t delta) {
  if (BGM_Enabled()) {
    BGM_ChangeMIDIDevice(delta);
  }
}

void MidiPanel::FnFixes(MenuController &, int_fast8_t) {
  const auto flags = (ConfigDat.MidFlags.v ^ MID_FLAGS::FIX_SYSEX_BUGS);
  ConfigDat.MidFlags.v = Mid_SetFlags(flags);
}

void MidiPanel::Refresh(MenuController &, bool tick) {
  const auto maybe_dev = MidBackend_DeviceName();
  if (maybe_dev) {
    if (tick) {
      marquee_time_ += 16;
    }
    const auto dev = maybe_dev.value();
    if (dev.size() > 18) {
      std::string buf = std::format("     {}     {}", dev.data(), dev.data());
      if (marquee_time_ == 0) {
        marquee_now_ = ((marquee_now_ + 1) % (dev.size() + 5));
      }
      title_port_.Format("{:.18}", std::string_view{buf}.substr(marquee_now_));
    } else {
      marquee_now_ = 0;
      title_port_.Set(dev.data());
    }
  } else {
    title_port_.Set(">");
  }
  items_[0].Title = title_port_.Lit();
  EnumFlagSet(items_[0].Flags, MenuFlags::DISABLED,
              static_cast<std::underlying_type_t<MenuFlags>>(!maybe_dev));

  const auto fixes = !!(ConfigDat.MidFlags.v & MID_FLAGS::FIX_SYSEX_BUGS);
  title_fixes_.Format("SC88ProFXCompat{}", CHOICE_OFF_ON_NARROW[fixes]);
  items_[1].Title = title_fixes_.Lit();
}

// ---------------------------------------------------------------------------
// SoundPanel
// ---------------------------------------------------------------------------

SoundPanel::SoundPanel() {
  items_.reserve(8);
  items_.emplace_back(title_se_.Lit(), "SEを鳴らすかどうかの設定", FnSE);
  items_.emplace_back(title_bgm_.Lit(), "BGMを鳴らすかどうかの設定", FnBGM);
  items_.emplace_back(title_se_vol_.Lit(), "効果音の音量", FnSEVol,
                      MenuFlags::FAST_REPEAT);
  items_.emplace_back(title_bgm_vol_.Lit(), "音楽の音量", FnBGMVol,
                      MenuFlags::FAST_REPEAT);
  items_.emplace_back(title_bgm_gain_.Lit(),
                      "毎に曲から音量の違うことが外します", FnBGMGain);
  items_.emplace_back(title_bgm_pack_.Lit(),
                      "収録のサントラをダウンロードします", FnBGMPack);
#ifdef SUPPORT_MIDI_BACKEND
  items_.emplace_back("MIDI", "Change MIDI playback options",
                      midi_panel_.Menu());
#endif
  items_.emplace_back(SubmenuExitItem);
  menu_ = MenuDef(std::span(items_),
                  [this](MenuController &c, bool t) { Refresh(c, t); });
}

void SoundPanel::FnSE(MenuController &, int_fast8_t) {
  if ((ConfigDat.SoundFlags.v & SNDF_SE_ENABLE) != 0) {
    ConfigDat.SoundFlags.v &= (~SNDF_SE_ENABLE);
    Snd_SECleanup();
  } else {
    ConfigDat.SoundFlags.v |= SNDF_SE_ENABLE;
    LoadSound();
  }
}

void SoundPanel::FnBGM(MenuController &, int_fast8_t) {
  if (BGM_Enabled()) {
    BGM_Cleanup();
  } else {
    if (BGM_Init()) {
      BGM_Switch(0);
    }
  }
}

void SoundPanel::FnSEVol(MenuController &, int_fast8_t delta) {
  ConfigDat.SEVolume.v =
      std::clamp((ConfigDat.SEVolume.v + delta), 0, int{VOLUME_MAX});
  Snd_UpdateVolumes();
}

void SoundPanel::FnBGMVol(MenuController &, int_fast8_t delta) {
  ConfigDat.BGMVolume.v =
      std::clamp((ConfigDat.BGMVolume.v + delta), 0, int{VOLUME_MAX});
  BGM_UpdateVolume();
}

void SoundPanel::FnBGMPack(MenuController &, int_fast8_t) {
  if (!BGM_PacksAvailable()) {
    SDL_OpenURL(UI.BGMPackSoundtrackURL);
  } else {
    UI.OpenBGMPack();
  }
}

void SoundPanel::FnBGMGain(MenuController &, int_fast8_t) {
  BGM_SetGainApply(!BGM_GainApply());
}

void SoundPanel::Refresh(MenuController &ctrl, bool) {
  const auto sound_active = (ConfigDat.SoundFlags.v & SNDF_SE_ENABLE);
  const auto bgm_active = BGM_Enabled();

  if ((!ctrl.Active()) || (ctrl.LastKey() == KEY_UP) ||
      (ctrl.LastKey() == KEY_DOWN)) {
    BGM_PacksAvailable(true);
  }

  const auto *const norm_choice = CHOICE_OFF_ON_NARROW[BGM_GainApply()];
  items_[2].SetActive(sound_active != 0); // ItemSEVol
  items_[3].SetActive(bgm_active);        // ItemBGMVol
  items_[4].SetActive(bgm_active && BGM_HasGainFactor());

  title_se_.Format("Sound  [{}]", CHOICE_USE[sound_active == 0]);
  title_bgm_.Format("BGM    [{}]", CHOICE_USE[!bgm_active]);
  title_se_vol_.Format("SoundVolume [ {:3} ]", ConfigDat.SEVolume.v);
  title_bgm_vol_.Format("BGMVolume   [ {:3} ]", ConfigDat.BGMVolume.v);
  title_bgm_gain_.Format("BGMVolNormalize{}", norm_choice);

  if (!BGM_PacksAvailable()) {
    title_bgm_pack_.Set("BGMPack[ Download ]");
    items_[5].Help = "収録のサントラをダウンロードします";
  } else if (ConfigDat.BGMPack.empty()) {
    title_bgm_pack_.Format("BGMPack[{}]", CHOICE_USE[1]);
    items_[5].Help = "BGMパックのメニューを開きます";
  } else {
    title_bgm_pack_.Set("BGMPack[   ....   ]");
    items_[5].Help = "BGMパックのメニューを開きます";
  }

  items_[0].Title = title_se_.Lit();
  items_[1].Title = title_bgm_.Lit();
  items_[2].Title = title_se_vol_.Lit();
  items_[3].Title = title_bgm_vol_.Lit();
  items_[4].Title = title_bgm_gain_.Lit();
  items_[5].Title = title_bgm_pack_.Lit();
}

// ---------------------------------------------------------------------------
// PadPanel
// ---------------------------------------------------------------------------

PadPanel::PadPanel() {
  static constexpr const char *HELP = "パッド上のボタンを押すと変更";
  items_.reserve(5);
  items_.emplace_back(titles_[0].Lit(), HELP, Fn<ConfigDat.PadTama.v>);
  items_.emplace_back(titles_[1].Lit(), HELP, Fn<ConfigDat.PadBomb.v>);
  items_.emplace_back(titles_[2].Lit(), HELP, Fn<ConfigDat.PadShift.v>);
  items_.emplace_back(titles_[3].Lit(), HELP, Fn<ConfigDat.PadCancel.v>);
  items_.emplace_back(SubmenuExitItem);
  menu_ = MenuDef(std::span(items_),
                  [this](MenuController &c, bool t) { Refresh(c, t); });
}

template <INPUT_PAD_BUTTON &ConfigPad>
bool PadPanel::Fn(MenuController &ctrl, INPUT_BITS key) {
  key &= (~Pad_Data);
  const auto temp = Key_PadSingle();
  if (temp) {
    ConfigPad = temp.value();
    ctrl.SearchActive()->SetItems(ctrl, false);
  }
  return !Input_IsOK(key);
}

void PadPanel::Refresh(MenuController &, bool) {
  static constexpr std::array<std::string_view, 4> labels = {
      "Shot", "Bomb", "SpeedDown", "ESC"};
  const auto max_len = std::ranges::max_element(labels, [](auto a, auto b) {
                         return a.length() < b.length();
                       })->length();

  auto set = [max_len](MenuText &buf, std::string_view label,
                       INPUT_PAD_BUTTON v) {
    if (v > 0) {
      buf.Format("{:>{}}[Button{:2}]", label, max_len, v);
    } else {
      buf.Format("{:>{}}[--------]", label, max_len);
    }
  };

  set(titles_[0], labels[0], ConfigDat.PadTama.v);
  set(titles_[1], labels[1], ConfigDat.PadBomb.v);
  set(titles_[2], labels[2], ConfigDat.PadShift.v);
  set(titles_[3], labels[3], ConfigDat.PadCancel.v);

  for (size_t i = 0; i < 4; i++) {
    items_[i].Title = titles_[i].Lit();
  }
}

// ---------------------------------------------------------------------------
// InputPanel
// ---------------------------------------------------------------------------

InputPanel::InputPanel() {
  items_.reserve(4);
  items_.emplace_back(titles_[0].Lit(), "弾キーのメッセージスキップ設定",
                      FnMsgSkip);
  items_.emplace_back(titles_[1].Lit(), "弾キーの押しっぱなしで低速移動",
                      FnZSpeedDown);
  items_.emplace_back("Joy Pad", "パッドの設定をします", pad_.Menu());
  items_.emplace_back(SubmenuExitItem);
  menu_ = MenuDef(std::span(items_),
                  [this](MenuController &c, bool t) { Refresh(c, t); });
}

void InputPanel::FnMsgSkip(MenuController &, int_fast8_t) {
  ConfigDat.InputFlags.v ^= INPF_Z_MSKIP_ENABLE;
}

void InputPanel::FnZSpeedDown(MenuController &, int_fast8_t) {
  ConfigDat.InputFlags.v ^= INPF_Z_SPDDOWN_ENABLE;
}

void InputPanel::Refresh(MenuController &, bool) {
  const auto skip = ((ConfigDat.InputFlags.v & INPF_Z_MSKIP_ENABLE) != 0);
  const auto down = ((ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) != 0);

  titles_[0].Format("Z-MessageSkip[{}]", (skip ? "ＯＫ" : "禁止"));
  titles_[1].Format("Z-SpeedDown  [{}]", (down ? "ＯＫ" : "禁止"));

  items_[0].Title = titles_[0].Lit();
  items_[1].Title = titles_[1].Lit();
}

// ---------------------------------------------------------------------------
// ConfigPanel
// ---------------------------------------------------------------------------

ConfigPanel::ConfigPanel() {
  items_.reserve(5);
  items_.emplace_back(" Difficulty", "難易度に関する設定", difficulty_.Menu());
  items_.emplace_back(" Graphic", "グラフィックに関する設定", graphics_.Menu());
  items_.emplace_back(" Sound / Music", "ＳＥ／ＢＧＭに関する設定",
                      sound_.Menu());
  items_.emplace_back(" Input", "入力デバイスに関する設定", input_.Menu());
  items_.emplace_back(" Exit", HELP_SUBMENU_EXIT, CWinExitFn);
  menu_ = MenuDef(std::span(items_));
}

void ConfigPanel::Init() { graphics_.Init(); }

// ---------------------------------------------------------------------------
// MainMenuPanel
// ---------------------------------------------------------------------------

MainMenuPanel::MainMenuPanel() : title_("     Main Menu") {
  items_.reserve(7);
  items_.emplace_back("   Game  Start", "ゲームを開始します", FnGameStart);
  items_.emplace_back("   Extra Start", "ゲームを開始します(Extra)", FnExStart);
  items_.emplace_back("   Replay Files", "リプレイファイルの管理",
                      ReplayFilesMenuOpen);
  items_.emplace_back("   Config", "各種設定を変更します", config_.Menu());
  items_.emplace_back("   Score", "スコアの表示をします", FnScore);
  items_.emplace_back("   Music", "音楽室に入ります", FnMusic);
  items_.emplace_back("   Exit", "ゲームを終了します", CWinExitFn);
  menu_ = MenuDef(
      std::span(items_), [this](MenuController &c, bool t) { Refresh(c, t); },
      &title_);
}

void MainMenuPanel::Init() { config_.Init(); }

bool MainMenuPanel::FnGameStart(MenuController &, INPUT_BITS key) {
  if (Input_IsOK(key)) {
    GameFlow.WeaponSelectInit(false);
  }
  return true;
}

bool MainMenuPanel::FnExStart(MenuController &, INPUT_BITS key) {
  if (Input_IsOK(key)) {
    if (ConfigDat.ExtraStgFlags.v != 0U) {
      GameFlow.WeaponSelectInit(true);
    }
  }
  return true;
}

bool MainMenuPanel::FnScore(MenuController &, INPUT_BITS key) {
  if (Input_IsOK(key)) {
    ScoreNameInit();
  }
  return true;
}

bool MainMenuPanel::FnMusic(MenuController &, INPUT_BITS key) {
  if (Input_IsOK(key)) {
    UI.MsgForceClose();
    MusicRoomInit();
  }
  return true;
}

bool MainMenuPanel::ReplayFilesMenuOpen(MenuController &, INPUT_BITS key) {
  if (Input_IsOK(key)) {
    UI.OpenReplayFiles();
  }
  return true;
}

void MainMenuPanel::Refresh(MenuController &, bool) {
  items_[5].SetActive(BGM_Enabled()); // ItemMusic
}
