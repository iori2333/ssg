/*                                                                           */
/*   GameMain.h   ウィンドウシステム切り替えなどの処理                       */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

// 2000/02/03 : 製作開始

///// [Include Files] /////
#include "ENDING.h"

///// [ 定数 ] /////
///// [マクロ] /////
///// [構造体] /////

///// [グローバル変数] /////
extern bool IsDemoplay;

///// [関数] /////
// ゲーム進行用関数ポインタ(WinMainからコールする)
extern void (*GameMain)(bool &quit);

bool WeaponSelectInit(bool ExStg);
bool GameInit(void (*NextProc)(bool &quit)); // ゲームの初期化をする
void GameRestart(void);               // ゲームを再開する(ESC 抜けから)
bool GameExit(bool bNeedChgMusic = true); // ゲームから抜ける
void GameOverInit(void);                  // ゲームオーバーの前処理
void GameContinue(void);                  // コンティニューを行う場合

bool GameReplayInitAll(const char8_t *fn); // マルチステージリプレイ用の初期化

bool SProjectInit(void); // 西方Ｐｒｏｊｅｃｔ表示の初期化

bool GameExstgInit(void); // エキストラステージを始める

bool NameRegistInit(bool bNeedChgMusic); // お名前入力の初期化
bool ScoreNameInit(void);                // お名前表示画面

bool GameNextStage(void); // 次のステージに移行する
