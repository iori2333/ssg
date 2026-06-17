/*                                                                           */
/*   ui_manager.cpp   UI マネージャ                                           */
/*                                                                           */
/*                                                                           */

#include "ui_manager.h"

#include "window_ctrl.h"

UIManager UI;

UIManager::UIManager() = default;

void UIManager::Bind(MenuController &main, MenuController &exit_w,
                     MenuController &continue_w, MenuController &bgm_pack,
                     MenuController &game_over_save,
                     MenuController &replay_files) {
  main_ = &main;
  exit_ = &exit_w;
  continue_ = &continue_w;
  bgm_pack_ = &bgm_pack;
  game_over_save_ = &game_over_save;
  replay_files_ = &replay_files;
}

// タイトル画面でアクティブなメニューコントローラを返す。
MenuController *UIManager::ActiveMenu() {
  if ((replay_files_ != nullptr) && replay_files_->Active()) {
    return replay_files_;
  }
  if ((bgm_pack_ != nullptr) && bgm_pack_->Active()) {
    return bgm_pack_;
  }
  return main_;
}

// タイトル画面のアクティブメニューにヘルプ文字列を送る。
void UIManager::MsgHelp() {
  if (auto *active = ActiveMenu()) {
    msg_window_.Help(active);
  }
}
