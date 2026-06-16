/*                                                                           */
/*   WINDOWCTRL.h   ウィンドウの定義＆管理                                   */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

//

///// [Include Files] /////

///// [ 定数 ] /////
///// [マクロ] /////
///// [構造体] /////

///// [グローバル変数] /////
extern struct tagWINDOW_SYSTEM MainWindow;
extern struct tagWINDOW_SYSTEM BGMPackWindow;
extern struct tagWINDOW_SYSTEM ExitWindow;
extern struct tagWINDOW_SYSTEM ContinueWindow;
extern struct tagWINDOW_SYSTEM GameOverSaveWindow;
extern struct tagWINDOW_SYSTEM ReplayFilesWindow;

///// [関数] /////
void InitMainWindow();     // メインメニューの初期化
void InitExitWindow();     // 終了Ｙ／Ｎウィンドウの初期化
void InitContinueWindow(); // コンティニューＹ／Ｎウィンドウの初期化
