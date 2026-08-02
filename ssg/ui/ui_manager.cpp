///
/// UiManager - UI manager
///

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "menu/menu_builder.h"
#include "menu/menu_tree.h"
#include "msg_window/msg_window.h"
#include "ui_manager.h"

#include "gfx/constants.h"
#include "gfx/coords.h"
#include "i18n/localization.h"
#include "settings/config.h"

namespace {

menu::MenuText Localized(i18n::Localization &localization,
                         std::string_view key) {
  const auto id = i18n::TextIdFromKey(key);
  return {[&localization, id] { return localization.Text(id); }};
}

} // namespace

UiManager::UiManager(audio::AudioSystem &audio)
    : boss_health_(audio), main_window_(audio), exit_window_(audio),
      game_over_window_(audio) {}

void UiManager::ConfigureMain(ConfigData &config,
                              menu::MainMenuServices services) {
  auto &localization = services.localization;
  root_menu_ = menu::BuildMainMenuTree(
      config, services,
      [this](menu::MainMenuAction action) { main_menu_action_ = action; });

  std::vector<std::unique_ptr<menu::IMenuNode>> exit_items;
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      Localized(localization, "ui.pause.cancel"), "",
      [this](menu::MenuController &) {
        pause_action_ = PauseAction::Resume;
        return false;
      }));

  auto save_and_exit = std::make_unique<menu::ActionNode>(
      Localized(localization, "ui.pause.save_and_exit"), "",
      [this](menu::MenuController &) {
        pause_action_ = PauseAction::SaveReplayAndExit;
        return false;
      });
  save_and_exit_item_ = save_and_exit.get();
  exit_items.push_back(std::move(save_and_exit));
  exit_items.push_back(std::make_unique<menu::ActionNode>(
      Localized(localization, "ui.pause.confirm_exit"), "",
      [this](menu::MenuController &) {
        pause_action_ = PauseAction::Exit;
        return false;
      }));
  exit_menu_ = std::make_unique<menu::EntryNode>(
      Localized(localization, "ui.pause.title"), "", std::move(exit_items));

  std::vector<std::unique_ptr<menu::IMenuNode>> game_over_items;
  auto continue_game = std::make_unique<menu::ActionNode>(
      Localized(localization, "ui.game_over.continue"), "",
      [this](menu::MenuController &) {
        game_over_action_ = GameOverAction::Continue;
        return false;
      });
  continue_item_ = continue_game.get();
  game_over_items.push_back(std::move(continue_game));

  auto save_replay = std::make_unique<menu::ActionNode>(
      Localized(localization, "ui.game_over.save_replay_exit"), "",
      [this](menu::MenuController &) {
        game_over_action_ = GameOverAction::SaveReplayAndExit;
        return false;
      });
  save_replay_item_ = save_replay.get();
  game_over_items.push_back(std::move(save_replay));

  game_over_items.push_back(std::make_unique<menu::ActionNode>(
      Localized(localization, "ui.game_over.exit_without_replay"), "",
      [this](menu::MenuController &) {
        game_over_action_ = GameOverAction::Exit;
        return false;
      }));
  game_over_menu_ = std::make_unique<menu::EntryNode>(
      Localized(localization, "ui.game_over.title"), "",
      std::move(game_over_items));

  auto exit_title = Localized(localization, "ui.common.back");
  auto exit_help = Localized(localization, "ui.common.back_help");
  main_window_.SetExitText(exit_title, exit_help);
  exit_window_.SetExitText(exit_title, exit_help);
  game_over_window_.SetExitText(std::move(exit_title), std::move(exit_help));
}

void UiManager::InitMessageWindow(const WindowLtrb &rect,
                                  MsgWindowFlags flags) {
  msg_window_.Init(rect, flags);
}

void UiManager::OpenMessageWindow() { msg_window_.Open(); }

void UiManager::CloseMessageWindow() { msg_window_.Close(); }

void UiManager::ForceCloseMessageWindow() { msg_window_.ForceClose(); }

void UiManager::TickMessageWindow() { msg_window_.Tick(); }

void UiManager::DrawMessageWindow() { msg_window_.Draw(); }

void UiManager::ShowMessage(std::string_view message) {
  msg_window_.AppendMessage(message);
}

void UiManager::SetMessageFace(std::size_t face_id) {
  msg_window_.SetFace(face_id);
}

void UiManager::SetLargeMessageFont() { msg_window_.SetFont(FontId::Large); }

void UiManager::NewMessagePage() { msg_window_.NewPage(); }

void UiManager::InitMain() {
  main_menu_action_.reset();
  main_window_.Init(200);
  main_window_.Navigate(*root_menu_, 0);
}

void UiManager::InitExit() {
  exit_window_.Init(180);
  exit_window_.SetRootCancelEnabled(false);
  exit_window_.Navigate(*exit_menu_);
}

void UiManager::InitGameOver() {
  game_over_window_.Init(240);
  game_over_window_.SetRootCancelEnabled(false);
  game_over_window_.Navigate(*game_over_menu_);
}

void UiManager::PrepareExitMenu(bool can_save_replay) {
  pause_action_.reset();
  save_and_exit_item_->SetEnabled(can_save_replay);
}

void UiManager::PrepareGameOverMenu(bool can_continue, bool can_save_replay) {
  game_over_action_.reset();
  continue_item_->SetEnabled(can_continue);
  save_replay_item_->SetEnabled(can_save_replay);
}

std::optional<UiManager::PauseAction> UiManager::TakePauseAction() {
  return std::exchange(pause_action_, std::nullopt);
}

std::optional<UiManager::GameOverAction> UiManager::TakeGameOverAction() {
  return std::exchange(game_over_action_, std::nullopt);
}

std::optional<menu::MainMenuAction> UiManager::TakeMainMenuAction() {
  return std::exchange(main_menu_action_, std::nullopt);
}

void UiManager::ShowMenuHelp() {
  if (main_window_.Active()) {
    msg_window_.ShowHelp(main_window_.GetCurrentHelp());
  }
}
