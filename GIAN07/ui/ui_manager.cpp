///
/// UIManager - UI manager
///

#include <utility>
#include <vector>

#include "menu/menu_builder.h"
#include "ui_manager.h"

#include "audio/bgm.h"
#include "gameflow/gameflow_manager.h"
#include "settings/config.h"

UIManager::UIManager() {
  std::vector<std::unique_ptr<menu::IMenuNode>> exit_items;
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      "  Save && Exit  ", "", [](menu::MenuController &) {
        GameFlow.ctx.replay_scene.BeginSave(
            false, [](bool) { GameFlow.ctx.ui.on_game_exit(); });
        return false;
      }));
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      "   お っ け ～ ", "", [](menu::MenuController &) {
        GameFlow.ctx.replay.CancelRecording();
        GameFlow.ctx.ui.on_game_exit();
        return false;
      }));
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      "   だ め だ め", "", [](menu::MenuController &) {
        GameFlow.ctx.ui.on_game_restart();
        return false;
      }));
  exit_menu_ = std::make_unique<menu::EntryNode>("終了するの？", "",
                                                 std::move(exit_items));

  std::vector<std::unique_ptr<menu::IMenuNode>> continue_items;
  continue_items.push_back(std::make_unique<menu::ActionNode>(
      "   お っ け ～", "", [](menu::MenuController &) {
        GameFlow.ctx.ui.on_game_continue();
        return false;
      }));
  continue_items.push_back(std::make_unique<menu::ActionNode>(
      "   や だ や だ", "", [](menu::MenuController &) {
        GameFlow.ctx.ui.on_game_exit_no_save();
        return false;
      }));
  continue_menu_ = std::make_unique<menu::EntryNode>("Ｃｏｎｔｉｎｕｅ？", "",
                                                     std::move(continue_items));

  std::vector<std::unique_ptr<menu::IMenuNode>> game_over_save_items;
  game_over_save_items.push_back(std::make_unique<menu::ActionNode>(
      "   お っ け ～ ", "", [](menu::MenuController &) {
        GameFlow.ctx.replay_scene.BeginSave(
            false, [](bool) { GameFlow.ctx.ui.on_game_exit(); });
        return false;
      }));
  game_over_save_items.push_back(std::make_unique<menu::ActionNode>(
      "   や だ や だ", "", [](menu::MenuController &) {
        GameFlow.ctx.replay.CancelRecording();
        GameFlow.ctx.ui.on_game_exit();
        return false;
      }));
  game_over_save_menu_ = std::make_unique<menu::EntryNode>(
      "Save Replay?", "", std::move(game_over_save_items));
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

void UIManager::InitMain() {
  root_menu_ = menu::BuildMainMenuTree(GameFlow.ctx.config);
  main_window_.Init(200);
  main_window_.Navigate(*root_menu_, 0);
}

void UIManager::InitExit() {
  exit_window_.Init(140);
  exit_window_.SetRootCancelEnabled(false);
  exit_window_.Navigate(*exit_menu_);

  game_over_save_window_.Init(140);
  game_over_save_window_.SetRootCancelEnabled(false);
  game_over_save_window_.Navigate(*game_over_save_menu_);
}

void UIManager::InitContinue() {
  continue_window_.Init(140);
  continue_window_.SetRootCancelEnabled(false);
  continue_window_.Navigate(*continue_menu_);
}

void UIManager::ShowMenuHelp() {
  if (main_window_.Active()) {
    msg_window_.ShowHelp(main_window_.GetCurrentHelp());
  }
}
