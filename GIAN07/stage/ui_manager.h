/*                                                                           */
/*   ui_manager.h   UI マネージャ                                             */
/*                                                                           */
/*   メッセージウィンドウを集約し、[game_main.cpp] のアドホックな              */
/*   ウィンドウ選択ロジックを置き換える。                                      */
/*   MenuController インスタンスは [window_ctrl.cpp] が所有し、                */
/*   UIManager はポインタで参照する。                                          */
/*                                                                           */

#pragma once

#include "msg_window/msg_window.h"
#include "window_sys.h"

class MenuController;

class UIManager {
public:
  UIManager();

  // MenuController を登録する（window_ctrl.cpp の初期化時に呼ぶ）。
  void Bind(MenuController &main, MenuController &exit_w,
            MenuController &continue_w, MenuController &bgm_pack,
            MenuController &game_over_save, MenuController &replay_files);

  // --- メッセージウィンドウ ---
  MsgWindow &Msg() { return msg_window_; }

  // メッセージウィンドウを 1 フレーム動作させる。
  void MsgTick() { msg_window_.Tick(); }

  // メッセージウィンドウを描画する。
  void MsgDraw() { msg_window_.Draw(); }

  // メッセージウィンドウを強制クローズする。
  void MsgForceClose() { msg_window_.ForceClose(); }

  // --- アクティブメニュー ---
  // タイトル画面でアクティブなメニューコントローラを返す。
  MenuController *ActiveMenu();

  // タイトル画面のアクティブメニューにヘルプ文字列を送る。
  void MsgHelp();

private:
  MsgWindow msg_window_;

  MenuController *main_ = nullptr;
  MenuController *exit_ = nullptr;
  MenuController *continue_ = nullptr;
  MenuController *bgm_pack_ = nullptr;
  MenuController *game_over_save_ = nullptr;
  MenuController *replay_files_ = nullptr;
};

// グローバルインスタンス
extern UIManager UI;
