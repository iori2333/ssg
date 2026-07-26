///
/// UIManager - UI Manager
///

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../msg_window/msg_window.h"
#include "menu_controller.h"
#include "menu_tree.h"
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
  menu::MenuController &Main() { return main_window_; }
  ::MenuController &Exit() { return exit_window_; }
  ::MenuController &Continue() { return continue_window_; }
  ::MenuController &GameOverSave() { return game_over_save_window_; }

  // --- Initialization ---
  void InitMain();
  void InitExit();
  void InitContinue();

  // --- Active menu on title screen ---
  menu::MenuController *ActiveMenu() { return &main_window_; }
  void MsgHelp();

  // --- Replay file storage (for inline list) ---
  std::vector<std::string> &replay_files_storage() { return replay_files_; }

private:
  // --- Message window ---
  MsgWindow msg_window_;

  // --- Main menu ---
  std::unique_ptr<menu::IMenuNode> root_menu_;
  menu::MenuController main_window_;

  // --- Exit dialog ---
  MenuLabel exit_title_;
  MenuItem exit_items_[3];
  MenuDef exit_menu_;
  ::MenuController exit_window_{exit_menu_};

  // --- Continue dialog ---
  MenuLabel continue_title_;
  MenuItem continue_items_[2];
  MenuDef continue_menu_;
  ::MenuController continue_window_{continue_menu_};

  // --- GameOverSave dialog ---
  MenuLabel game_over_save_title_;
  MenuItem game_over_save_items_[2];
  MenuDef game_over_save_menu_;
  ::MenuController game_over_save_window_{game_over_save_menu_};

  // --- Replay files storage ---
  std::vector<std::string> replay_files_;
};
