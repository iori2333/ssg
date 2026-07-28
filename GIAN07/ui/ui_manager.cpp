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
  auto save_and_exit = std::make_unique<menu::ActionNode>(
      "  Save & Exit  ", "", [](menu::MenuController &) {
        const bool extra_stage = GameFlow.ctx.session.stage == StageId::Extra;
        GameFlow.ctx.replay_scene.BeginSave(
            extra_stage, [](bool) { GameFlow.ctx.ui.on_game_exit(); });
        return false;
      });
  auto *save_and_exit_item = save_and_exit.get();
  save_and_exit->SetPoll([save_and_exit_item] {
    save_and_exit_item->SetEnabled(GameFlow.ctx.records.HasRecordedStages());
  });
  exit_items.push_back(std::move(save_and_exit));
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      "   お っ け ～ ", "", [](menu::MenuController &) {
        GameFlow.ctx.records.CancelRecording();
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

  std::vector<std::unique_ptr<menu::IMenuNode>> game_over_items;
  auto continue_game = std::make_unique<menu::ActionNode>(
      "Continue", "", [](menu::MenuController &) {
        GameFlow.ctx.ui.on_game_continue();
        return false;
      });
  auto *continue_item = continue_game.get();
  continue_game->SetPoll([continue_item] {
    continue_item->SetEnabled(GameFlow.ctx.player.Credits() != 0U);
  });
  game_over_items.push_back(std::move(continue_game));

  auto save_replay = std::make_unique<menu::ActionNode>(
      "Save Replay & Exit", "", [](menu::MenuController &) {
        GameFlow.ctx.ui.on_game_over_exit(true);
        return false;
      });
  auto *save_replay_item = save_replay.get();
  save_replay->SetPoll([save_replay_item] {
    save_replay_item->SetEnabled(GameFlow.ctx.records.HasRecordedStages());
  });
  game_over_items.push_back(std::move(save_replay));

  game_over_items.push_back(std::make_unique<menu::ActionNode>(
      "Exit Without Replay", "", [](menu::MenuController &) {
        GameFlow.ctx.ui.on_game_over_exit(false);
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

void UIManager::InitMain() {
  root_menu_ = menu::BuildMainMenuTree(GameFlow.ctx.config);
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

void UIManager::ShowMenuHelp() {
  if (main_window_.Active()) {
    msg_window_.ShowHelp(main_window_.GetCurrentHelp());
  }
}
