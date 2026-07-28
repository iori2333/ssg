///
/// UIManager - UI manager
///

#include <utility>
#include <vector>

#include "menu/menu_builder.h"
#include "ui_manager.h"

#include "settings/config.h"

UIManager::UIManager() {
  std::vector<std::unique_ptr<menu::IMenuNode>> exit_items;
  auto save_and_exit = std::make_unique<menu::ActionNode>(
      "  Save & Exit  ", "", [this](menu::MenuController &) {
        pause_action_ = PauseAction::SaveReplayAndExit;
        return false;
      });
  save_and_exit_item_ = save_and_exit.get();
  exit_items.push_back(std::move(save_and_exit));
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      "   お っ け ～ ", "", [this](menu::MenuController &) {
        pause_action_ = PauseAction::Exit;
        return false;
      }));
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      "   だ め だ め", "", [this](menu::MenuController &) {
        pause_action_ = PauseAction::Resume;
        return false;
      }));
  exit_menu_ = std::make_unique<menu::EntryNode>("終了するの？", "",
                                                 std::move(exit_items));

  std::vector<std::unique_ptr<menu::IMenuNode>> game_over_items;
  auto continue_game = std::make_unique<menu::ActionNode>(
      "Continue", "", [this](menu::MenuController &) {
        game_over_action_ = GameOverAction::Continue;
        return false;
      });
  continue_item_ = continue_game.get();
  game_over_items.push_back(std::move(continue_game));

  auto save_replay = std::make_unique<menu::ActionNode>(
      "Save Replay & Exit", "", [this](menu::MenuController &) {
        game_over_action_ = GameOverAction::SaveReplayAndExit;
        return false;
      });
  save_replay_item_ = save_replay.get();
  game_over_items.push_back(std::move(save_replay));

  game_over_items.push_back(std::make_unique<menu::ActionNode>(
      "Exit Without Replay", "", [this](menu::MenuController &) {
        game_over_action_ = GameOverAction::Exit;
        return false;
      }));
  game_over_menu_ = std::make_unique<menu::EntryNode>(
      "Game Over", "", std::move(game_over_items));
}

void UIManager::InitMessageWindow(const WINDOW_LTRB &rect,
                                  MsgWindowFlags flags) {
  msg_window_.Init(rect, flags);
}

void UIManager::OpenMessageWindow() { msg_window_.Open(); }

void UIManager::CloseMessageWindow() { msg_window_.Close(); }

void UIManager::ForceCloseMessageWindow() { msg_window_.ForceClose(); }

void UIManager::TickMessageWindow() { msg_window_.Tick(); }

void UIManager::DrawMessageWindow() { msg_window_.Draw(); }

void UIManager::ShowMessage(std::string_view message) {
  msg_window_.AppendMessage(message);
}

void UIManager::SetMessageFace(uint8_t face_id) {
  msg_window_.SetFace(face_id);
}

void UIManager::SetLargeMessageFont() { msg_window_.SetFont(FONT_ID::LARGE); }

void UIManager::NewMessagePage() { msg_window_.NewPage(); }

void UIManager::InitMain(ConfigData &config, menu::MainMenuServices services) {
  main_menu_action_.reset();
  root_menu_ = menu::BuildMainMenuTree(
      config, services,
      [this](menu::MainMenuAction action) { main_menu_action_ = action; });
  main_window_.Init(200);
  main_window_.Navigate(*root_menu_, 0);
}

void UIManager::InitExit() {
  exit_window_.Init(140);
  exit_window_.SetRootCancelEnabled(false);
  exit_window_.Navigate(*exit_menu_);
}

void UIManager::InitGameOver() {
  game_over_window_.Init(240);
  game_over_window_.SetRootCancelEnabled(false);
  game_over_window_.Navigate(*game_over_menu_);
}

void UIManager::PrepareExitMenu(bool can_save_replay) {
  pause_action_.reset();
  save_and_exit_item_->SetEnabled(can_save_replay);
}

void UIManager::PrepareGameOverMenu(bool can_continue, bool can_save_replay) {
  game_over_action_.reset();
  continue_item_->SetEnabled(can_continue);
  save_replay_item_->SetEnabled(can_save_replay);
}

std::optional<UIManager::PauseAction> UIManager::TakePauseAction() {
  return std::exchange(pause_action_, std::nullopt);
}

std::optional<UIManager::GameOverAction> UIManager::TakeGameOverAction() {
  return std::exchange(game_over_action_, std::nullopt);
}

std::optional<menu::MainMenuAction> UIManager::TakeMainMenuAction() {
  return std::exchange(main_menu_action_, std::nullopt);
}

void UIManager::ShowMenuHelp() {
  if (main_window_.Active()) {
    msg_window_.ShowHelp(main_window_.GetCurrentHelp());
  }
}
