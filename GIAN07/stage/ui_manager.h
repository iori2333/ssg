/*                                                                           */
/*   ui_manager.h   UI マネージャ                                             */
/*                                                                           */
/*   すべてのメニューコントローラ、スクロールメニュー、パネル、               */
/*   メッセージウィンドウを集約所有する。                                       */
/*   グローバル変数は [UI] のみ。                                               */
/*                                                                           */

#pragma once

#include "menu/panels.h"
#include "menu/scroll_menu.h"
#include "msg_window/msg_window.h"
#include "window_sys.h"

#include <string>
#include <vector>

class UIManager {
public:
  UIManager();

  // --- メッセージウィンドウ ---
  MsgWindow &Msg() { return msg_window_; }
  void MsgTick() { msg_window_.Tick(); }
  void MsgDraw() { msg_window_.Draw(); }
  void MsgForceClose() { msg_window_.ForceClose(); }

  // --- メニューウィンドウアクセス ---
  MenuController &Main() { return main_window_; }
  MenuController &Exit() { return exit_window_; }
  MenuController &Continue() { return continue_window_; }
  MenuController &GameOverSave() { return game_over_save_window_; }

  // --- 初期化 ---
  void InitMain();
  void InitExit();
  void InitContinue();

  // --- スクロールメニューを開く ---
  void OpenBGMPack();
  void OpenReplayFiles();

  // BGM Pack サウンドトラックのダウンロード URL
  static constexpr const char *BGMPackSoundtrackURL =
      "https://github.com/nmlgc/BGMPacks/releases/tag/2024-10-05";

  // --- タイトル画面のアクティブメニュー ---
  MenuController *ActiveMenu();
  void MsgHelp();

private:
  // BGM Pack スクロールメニュー callback
  size_t BGMPackListSize();
  void BGMPackGenerate(MenuItem &ret, size_t generated, size_t selected);
  bool BGMPackHandle(MenuController &ctrl, INPUT_BITS key, size_t selected);

  // Replay Files スクロールメニュー callback
  size_t ReplayFilesListSize();
  void ReplayFilesGenerate(MenuItem &ret, size_t generated, size_t selected);
  bool ReplayFilesHandle(MenuController &ctrl, INPUT_BITS key,
                         size_t selected);

  // --- メッセージウィンドウ ---
  MsgWindow msg_window_;

  // --- メインメニュー ---
  MainMenuPanel main_panel_;
  MenuController main_window_;

  // --- Exit ダイアログ ---
  MenuLabel exit_title_;
  MenuItem exit_items_[3];
  MenuDef exit_menu_;
  MenuController exit_window_;

  // --- Continue ダイアログ ---
  MenuLabel continue_title_;
  MenuItem continue_items_[2];
  MenuDef continue_menu_;
  MenuController continue_window_;

  // --- GameOverSave ダイアログ ---
  MenuLabel game_over_save_title_;
  MenuItem game_over_save_items_[2];
  MenuDef game_over_save_menu_;
  MenuController game_over_save_window_;

  // --- BGM Pack スクロールメニュー状態 ---
  MenuText bgm_title_text_;
  MenuLabel bgm_title_item_;
  std::vector<std::u8string> bgm_packs_;
  size_t bgm_sel_at_open_ = 0;
  ScrollMenu bgm_pack_scroll_menu_;
  MenuController bgm_pack_window_;

  // --- Replay Files スクロールメニュー状態 ---
  MenuText replay_title_text_;
  MenuLabel replay_title_item_;
  std::vector<std::u8string> replay_files_;
  ScrollMenu replay_files_scroll_menu_;
  MenuController replay_files_window_;
};

// 唯一のグローバルインスタンス
extern UIManager UI;
