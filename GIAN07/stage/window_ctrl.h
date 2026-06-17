/*                                                                           */
/*   WINDOWCTRL.h   ウィンドウの定義＆管理                                   */
/*                                                                           */
/*                                                                           */

#pragma once

#include "ui_manager.h"
#include "window_sys.h"

///// [更新履歴] /////

//

///// [Include Files] /////

///// [ 定数 ] /////
///// [マクロ] /////
///// [構造体] /////

///// [グローバル変数] /////
extern MenuController MainWindow;
extern MenuController BGMPackWindow;
extern MenuController ExitWindow;
extern MenuController ContinueWindow;
extern MenuController GameOverSaveWindow;
extern MenuController ReplayFilesWindow;

///// [関数] /////
void InitMainWindow();     // メインメニューの初期化
void InitExitWindow();     // 終了Ｙ／Ｎウィンドウの初期化
void InitContinueWindow(); // コンティニューＹ／Ｎウィンドウの初期化
