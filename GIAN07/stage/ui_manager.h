///
/// UIManager - UI Manager
///

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "menu/panels.h"
#include "menu/scroll_menu.h"
#include "msg_window/msg_window.h"
#include "window_sys.h"

class UIManager {
public:
  UIManager();

  // --- Game flow callbacks (injected by gameflow layer) ---
  std::function<void()> on_game_exit;
  std::function<void()> on_game_exit_no_save;
  std::function<void()> on_game_restart;
  std::function<void()> on_game_continue;

  // --- Message window ---
  MsgWindow &Msg() { return msg_window_; }
  void MsgTick() { msg_window_.Tick(); }
  void MsgDraw() { msg_window_.Draw(); }
  void MsgForceClose() { msg_window_.ForceClose(); }

  // --- Menu window access ---
  MenuController &Main() { return main_window_; }
  MenuController &Exit() { return exit_window_; }
  MenuController &Continue() { return continue_window_; }
  MenuController &GameOverSave() { return game_over_save_window_; }

  // --- Initialization ---
  void InitMain();
  void InitExit();
  void InitContinue();

  // --- Open scroll menu ---
  void OpenBGMPack();
  void OpenSoundFont();
  void OpenReplayFiles();

  // BGM Pack soundtrack download URL
  static constexpr const char *BGMPackSoundtrackURL =
      "https://github.com/nmlgc/BGMPacks/releases/tag/2024-10-05";

  // --- Title screen active menu ---
  MenuController *ActiveMenu();
  void MsgHelp();

private:
  // BGM Pack scroll menu callback
  size_t BGMPackListSize();
  void BGMPackGenerate(MenuItem &ret, size_t generated, size_t selected);
  bool BGMPackHandle(MenuController &ctrl, INPUT_BITS key, size_t selected);

  // SoundFont scroll menu callback
  size_t SoundFontListSize();
  void SoundFontGenerate(MenuItem &ret, size_t generated, size_t selected);
  bool SoundFontHandle(MenuController &ctrl, INPUT_BITS key, size_t selected);

  // Replay Files scroll menu callback
  size_t ReplayFilesListSize();
  void ReplayFilesGenerate(MenuItem &ret, size_t generated, size_t selected);
  bool ReplayFilesHandle(MenuController &ctrl, INPUT_BITS key, size_t selected);

  // --- Message window ---
  MsgWindow msg_window_;

  // --- Main menu ---
  MainMenuPanel main_panel_;
  MenuController main_window_;

  // --- Exit dialog ---
  MenuLabel exit_title_;
  MenuItem exit_items_[3];
  MenuDef exit_menu_;
  MenuController exit_window_;

  // --- Continue dialog ---
  MenuLabel continue_title_;
  MenuItem continue_items_[2];
  MenuDef continue_menu_;
  MenuController continue_window_;

  // --- GameOverSave dialog ---
  MenuLabel game_over_save_title_;
  MenuItem game_over_save_items_[2];
  MenuDef game_over_save_menu_;
  MenuController game_over_save_window_;

  // --- BGM Pack scroll menu state ---
  MenuText bgm_title_text_;
  MenuLabel bgm_title_item_;
  std::vector<std::string> bgm_packs_;
  size_t bgm_sel_at_open_ = 0;
  ScrollMenu bgm_pack_scroll_menu_;
  MenuController bgm_pack_window_;

  // --- SoundFont scroll menu state ---
  MenuText sf_title_text_;
  MenuLabel sf_title_item_;
  std::vector<std::string> sf_labels_;
  ScrollMenu sf_scroll_menu_;
  MenuController sf_window_;

  // --- Replay Files scroll menu state ---
  MenuText replay_title_text_;
  MenuLabel replay_title_item_;
  std::vector<std::string> replay_files_;
  ScrollMenu replay_files_scroll_menu_;
  MenuController replay_files_window_;
};

// Single global instance
extern UIManager UI;
