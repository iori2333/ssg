///
/// UiManager - UI Manager
///

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "gameplay/boss_health_gauge.h"
#include "gameplay/gameplay_hud.h"
#include "menu/menu_builder.h"
#include "menu/menu_controller.h"
#include "menu/menu_tree.h"
#include "msg_window/msg_window.h"

struct ConfigData;

namespace audio {
class AudioSystem;
}

class UiManager {
public:
  explicit UiManager(audio::AudioSystem &audio);

  enum class PauseAction : uint8_t { SaveReplayAndExit, Exit, Resume };
  enum class GameOverAction : uint8_t { Continue, SaveReplayAndExit, Exit };

  // --- Message window ---
  void InitMessageWindow(const WindowLtrb &rect,
                         MsgWindowFlags flags = MsgWindowFlags::None);
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
  static void DrawTopHud(const GameplayHudModel &model) {
    GameplayHud::DrawTop(model);
  }
  void DrawSidebarHud(const GameplayHudModel &model) {
    gameplay_hud_.DrawSidebars(model);
  }
  void UpdateBossHud(const BossHudModel &model) { boss_health_.Sync(model); }
  void DrawBossHud(uint32_t stage_frame) { boss_health_.Draw(stage_frame); }

  // --- Menu window access ---
  menu::MenuController &Main() { return main_window_; }
  menu::MenuController &Exit() { return exit_window_; }
  menu::MenuController &GameOver() { return game_over_window_; }

  // --- Initialization ---
  void ConfigureMain(ConfigData &config, menu::MainMenuServices services);
  void InitMain();
  void InitExit();
  void InitGameOver();
  void PrepareExitMenu(bool can_save_replay);
  void PrepareGameOverMenu(bool can_continue, bool can_save_replay);
  [[nodiscard]] std::optional<PauseAction> TakePauseAction();
  [[nodiscard]] std::optional<GameOverAction> TakeGameOverAction();
  [[nodiscard]] std::optional<menu::MainMenuAction> TakeMainMenuAction();

  void ShowMenuHelp();

private:
  // --- Message window ---
  MsgWindow msg_window_;
  GameplayHud gameplay_hud_;
  BossHealthGauge boss_health_;

  // --- Main menu ---
  std::unique_ptr<menu::IMenuNode> root_menu_;
  menu::MenuController main_window_;
  std::optional<menu::MainMenuAction> main_menu_action_;

  // --- Exit dialog ---
  std::unique_ptr<menu::IMenuNode> exit_menu_;
  menu::MenuController exit_window_;
  menu::ActionNode *save_and_exit_item_ = nullptr;
  std::optional<PauseAction> pause_action_;

  // --- Game over dialog ---
  std::unique_ptr<menu::IMenuNode> game_over_menu_;
  menu::MenuController game_over_window_;
  menu::ActionNode *continue_item_ = nullptr;
  menu::ActionNode *save_replay_item_ = nullptr;
  std::optional<GameOverAction> game_over_action_;
};
