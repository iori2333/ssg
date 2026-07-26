///
/// UIManager - UI manager
///

#include "ui_manager.h"

#include "audio/bgm.h"
#include "core/config.h"
#include "gameflow/gameflow_manager.h"
#include "menu_builder.h"

UIManager::UIManager()
    : exit_title_("    終了するの？"),
      exit_items_{
          MenuItem{"  Save && Exit  ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.demos.SaveReplayAll(false);
                       GameFlow.ctx.ui.on_game_exit();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   お っ け ～ ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.demos.save_all_enable = false;
                       GameFlow.ctx.ui.on_game_exit();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   だ め だ め", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.ui.on_game_restart();
                       return false;
                     }
                     return true;
                   }},
      },
      exit_menu_(std::span(exit_items_), [](MenuController &, bool) {},
                 &exit_title_),

      continue_title_(" Ｃｏｎｔｉｎｕｅ？"),
      continue_items_{
          MenuItem{"   お っ け ～", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.ui.on_game_continue();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   や だ や だ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.ui.on_game_exit_no_save();
                       return false;
                     }
                     return true;
                   }},
      },
      continue_menu_(std::span(continue_items_), [](MenuController &, bool) {},
                     &continue_title_),

      game_over_save_title_("  Save Replay?"),
      game_over_save_items_{
          MenuItem{"   お っ け ～ ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.demos.SaveReplayAll(false);
                       GameFlow.ctx.ui.on_game_exit();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   や だ や だ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.ctx.demos.save_all_enable = false;
                       GameFlow.ctx.ui.on_game_exit();
                       return false;
                     }
                     return true;
                   }},
      },
      game_over_save_menu_(std::span(game_over_save_items_),
                           [](MenuController &, bool) {},
                           &game_over_save_title_) {}

void UIManager::InitMain() {
  root_menu_ = menu::BuildMainMenuTree(*GameFlow.ctx.cfg);
  main_window_.Init(200);
  main_window_.Navigate(*root_menu_, 0);
}

void UIManager::InitExit() {
  exit_window_.Init(140);
  game_over_save_window_.Init(140);
}

void UIManager::InitContinue() { continue_window_.Init(140); }

void UIManager::MsgHelp() {
  if (main_window_.Active()) {
    msg_window_.HelpStr(main_window_.GetCurrentHelp().data());
  }
}
