///
/// UIManager - UI Manager
///

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "gameplay/boss_health_gauge.h"
#include "gameplay/gameplay_hud.h"
#include "menu/menu_controller.h"
#include "menu/menu_tree.h"
#include "msg_window/msg_window.h"

class UIManager {
public:
  UIManager();

  // --- Game flow callbacks (injected by gameflow layer) ---
  std::function<void()> on_game_exit;
  std::function<void()> on_game_exit_no_save;
  std::function<void()> on_game_restart;
  std::function<void()> on_game_continue;

  // --- Message window ---
  void InitMessageWindow(const WINDOW_LTRB &rect,
                         MsgWindowFlags flags = MsgWindowFlags::NONE);
  void OpenMessageWindow();
  void CloseMessageWindow();
  void ForceCloseMessageWindow();
  void TickMessageWindow();
  void DrawMessageWindow();
  void ShowMessage(std::string_view message);
  void SetMessageFace(uint8_t face_id);
  void SetLargeMessageFont();
  void NewMessagePage();

  // --- Gameplay HUD ---
  void DrawTopHud(const GameplayHudModel &model) {
    gameplay_hud_.DrawTop(model);
  }
  void DrawSidebarHud(const GameplayHudModel &model) {
    gameplay_hud_.DrawSidebars(model);
  }
  void UpdateBossHud(const BossHudModel &model) { boss_health_.Sync(model); }
  void DrawBossHud(uint32_t stage_frame) { boss_health_.Draw(stage_frame); }

  // --- Menu window access ---
  menu::MenuController &Main() { return main_window_; }
  menu::MenuController &Exit() { return exit_window_; }
  menu::MenuController &Continue() { return continue_window_; }
  menu::MenuController &GameOverSave() { return game_over_save_window_; }

  // --- Initialization ---
  void InitMain();
  void InitExit();
  void InitContinue();

  // --- Active menu on title screen ---
  menu::MenuController *ActiveMenu() { return &main_window_; }
  void ShowMenuHelp();

  // --- Replay file storage (for inline list) ---
  std::vector<std::string> &replay_files_storage() { return replay_files_; }

private:
  // --- Message window ---
  MsgWindow msg_window_;
  GameplayHud gameplay_hud_;
  BossHealthGauge boss_health_;

  // --- Main menu ---
  std::unique_ptr<menu::IMenuNode> root_menu_;
  menu::MenuController main_window_;

  // --- Exit dialog ---
  std::unique_ptr<menu::IMenuNode> exit_menu_;
  menu::MenuController exit_window_;

  // --- Continue dialog ---
  std::unique_ptr<menu::IMenuNode> continue_menu_;
  menu::MenuController continue_window_;

  // --- GameOverSave dialog ---
  std::unique_ptr<menu::IMenuNode> game_over_save_menu_;
  menu::MenuController game_over_save_window_;

  // --- Replay files storage ---
  std::vector<std::string> replay_files_;
};
