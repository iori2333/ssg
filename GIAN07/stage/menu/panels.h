///
/// Panels - Menu panel class declarations
///

#pragma once

#include "menu/menu_text.h"
#include "platform/sdl/graphics_sdl.h" // SUPPORT_GRP_*, GRP_SCREENSHOT_*
#include "window_sys.h"

#include <array>
#include <vector>

// ---------------------------------------------------------------------------
// Difficulty settings panel
// ---------------------------------------------------------------------------
class DifficultyPanel {
public:
  DifficultyPanel();
  MenuDef &Menu() { return menu_; }

private:
  void Refresh(MenuController &ctrl, bool tick);
  static void FnPlayerStock(MenuController &, int_fast8_t);
  static void FnBombStock(MenuController &, int_fast8_t);
  static void FnDifficulty(MenuController &, int_fast8_t);
  static void FnPracticeMode(MenuController &, int_fast8_t);
#ifdef PBG_DEBUG
  static void FnMsgDisplay(MenuController &, int_fast8_t);
  static void FnStgSelect(MenuController &, int_fast8_t);
  static void FnHit(MenuController &, int_fast8_t);
  static void FnDemo(MenuController &, int_fast8_t);
#endif

  std::array<MenuText, 8> titles_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Screenshot settings panel
// ---------------------------------------------------------------------------
class ScreenshotPanel {
public:
  ScreenshotPanel();
  MenuDef &Menu() { return menu_; }
  void RefreshActive(MenuController &ctrl);

private:
  void Refresh(MenuController &ctrl, bool tick);
  static void FnFormat(MenuController &, int_fast8_t);

  MenuText title_format_;
  std::array<MenuText, GRP_SCREENSHOT_EFFORT_COUNT> title_perf_;
  MenuText help_perf_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Graphics API settings panel
// ---------------------------------------------------------------------------
class ApiPanel {
public:
  ApiPanel();
  MenuDef &Menu() { return menu_; }

  // Build menu items based on available graphics APIs.
  void Init();

  static void FnDef(MenuController &, int_fast8_t);
  static void FnOverride(MenuController &, int_fast8_t);

private:
  void Refresh(MenuController &ctrl, bool tick);

  MenuText title_def_;
  MenuItem item_def_;
  std::array<MenuItem, 8> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Graphics settings panel
// ---------------------------------------------------------------------------
class GraphicsPanel {
public:
  GraphicsPanel();
  MenuDef &Menu() { return menu_; }

  ScreenshotPanel &Screenshot() { return screenshot_; }

  // Initialize submenu including API items.
  void Init();

private:
  void Refresh(MenuController &ctrl, bool tick);
  static void FnDisp(MenuController &, int_fast8_t);
  static void FnFSMode(MenuController &, int_fast8_t);
  static void FnScale(MenuController &, int_fast8_t);
  static void FnScMode(MenuController &, int_fast8_t);
  static void FnSkip(MenuController &, int_fast8_t);
  static void FnWinLocate(MenuController &, int_fast8_t);

  MenuText title_device_;
#ifdef SUPPORT_GRP_WINDOWED
  MenuText title_disp_;
  MenuText title_fs_mode_;
  MenuText help_fs_mode_;
#endif
#ifdef SUPPORT_GRP_SCALING
  MenuText title_scale_;
  MenuText title_sc_mode_;
  MenuText help_scale_;
  MenuText help_sc_mode_;
#endif
  MenuText title_skip_;
  MenuText title_msg_;

  ScreenshotPanel screenshot_;
#ifdef SUPPORT_GRP_API
  ApiPanel api_;
  MenuItem item_api_;
#endif

  std::vector<MenuItem> item_storage_;
  std::vector<MenuItem *> item_ptrs_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// MIDI settings panel
// ---------------------------------------------------------------------------
class MidiPanel {
public:
  MidiPanel();
  MenuDef &Menu() { return menu_; }

private:
  void Refresh(MenuController &ctrl, bool tick);
  static void FnDev(MenuController &, int_fast8_t);
  static void FnFixes(MenuController &, int_fast8_t);

  MenuText title_port_;
  MenuText title_fixes_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
  int marquee_now_ = 0;
  uint8_t marquee_time_ = 0;
};

// ---------------------------------------------------------------------------
// Sound settings panel
// ---------------------------------------------------------------------------
class SoundPanel {
public:
  SoundPanel();
  MenuDef &Menu() { return menu_; }

  void Refresh(MenuController &ctrl, bool tick);

private:
  static void FnSE(MenuController &, int_fast8_t);
  static void FnBGM(MenuController &, int_fast8_t);
  static void FnSEVol(MenuController &, int_fast8_t);
  static void FnBGMVol(MenuController &, int_fast8_t);
  static void FnBGMGain(MenuController &, int_fast8_t);
  static void FnBGMPack(MenuController &, int_fast8_t);

  MenuText title_se_;
  MenuText title_bgm_;
  MenuText title_se_vol_;
  MenuText title_bgm_vol_;
  MenuText title_bgm_gain_;
  MenuText title_bgm_pack_;
  MidiPanel midi_panel_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Pad input settings panel
// ---------------------------------------------------------------------------
class PadPanel {
public:
  PadPanel();
  MenuDef &Menu() { return menu_; }

private:
  void Refresh(MenuController &ctrl, bool tick);
  template <INPUT_PAD_BUTTON &ConfigPad>
  static bool Fn(MenuController &, INPUT_BITS);

  std::array<MenuText, 4> titles_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Input settings panel
// ---------------------------------------------------------------------------
class InputPanel {
public:
  InputPanel();
  MenuDef &Menu() { return menu_; }

private:
  void Refresh(MenuController &ctrl, bool tick);
  static void FnMsgSkip(MenuController &, int_fast8_t);
  static void FnZSpeedDown(MenuController &, int_fast8_t);

  std::array<MenuText, 2> titles_;
  PadPanel pad_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Settings panel (category selection)
// ---------------------------------------------------------------------------
class ConfigPanel {
public:
  ConfigPanel();
  MenuDef &Menu() { return menu_; }

  DifficultyPanel &Difficulty() { return difficulty_; }
  GraphicsPanel &Graphics() { return graphics_; }
  SoundPanel &Sound() { return sound_; }
  InputPanel &Input() { return input_; }

  // Initialize subpanels.
  void Init();

private:
  DifficultyPanel difficulty_;
  GraphicsPanel graphics_;
  SoundPanel sound_;
  InputPanel input_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};

// ---------------------------------------------------------------------------
// Main menu panel
// ---------------------------------------------------------------------------
class MainMenuPanel {
public:
  MainMenuPanel();
  MenuDef &Menu() { return menu_; }

  ConfigPanel &Config() { return config_; }
  SoundPanel &Sound() { return config_.Sound(); }

  // Initialize subpanels.
  void Init();

  void Refresh(MenuController &ctrl, bool tick);

private:
  static bool FnGameStart(MenuController &, INPUT_BITS);
  static bool FnExStart(MenuController &, INPUT_BITS);
  static bool FnMusic(MenuController &, INPUT_BITS);
  static bool FnScore(MenuController &, INPUT_BITS);
  static bool ReplayFilesMenuOpen(MenuController &, INPUT_BITS);

  MenuLabel title_;
  ConfigPanel config_;
  std::vector<MenuItem> items_;
  MenuDef menu_;
};
