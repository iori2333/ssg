/*                                                                           */
/*   WINDOWCTRL.h   ウィンドウの定義＆管理                                   */
/*                                                                           */
/*                                                                           */

#pragma once

#include "WindowSys.h"

///// [更新履歴] /////

//

///// [Include Files] /////

///// [ 定数 ] /////
///// [マクロ] /////
///// [構造体] /////

///// [グローバル変数] /////
extern WINDOW_SYSTEM MainWindow;
extern WINDOW_SYSTEM BGMPackWindow;
extern WINDOW_SYSTEM ExitWindow;
extern WINDOW_SYSTEM ContinueWindow;
extern WINDOW_SYSTEM GameOverSaveWindow;
extern WINDOW_SYSTEM ReplayFilesWindow;

///// [関数] /////
void InitMainWindow();     // メインメニューの初期化
void InitExitWindow();     // 終了Ｙ／Ｎウィンドウの初期化
void InitContinueWindow(); // コンティニューＹ／Ｎウィンドウの初期化
