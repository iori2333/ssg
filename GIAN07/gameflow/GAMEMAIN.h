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
// IsDemoplay → game_manager.h で extern bool& として再宣言
// GameMain, DemoTimer, DrawCount, WeaponKeyWait, GameOverTimer,
// CurrentName, CurrentRank, CurrentDif, VivTemp, InputLocked, FlashState
// → gameflow_manager.h で参照として宣言

///// [関数] /////

// WeaponSelectInit → inline wrapper in gameflow_manager.h
bool GameInit(void (*NextProc)(bool &quit)); // ゲームの初期化をする
void GameRestart(void);               // ゲームを再開する(ESC 抜けから)
bool GameExit(bool bNeedChgMusic = true); // ゲームから抜ける
void GameOverInit(void);                  // ゲームオーバーの前処理
void GameContinue(void);                  // コンティニューを行う場合

bool GameReplayInitAll(const char8_t *fn); // マルチステージリプレイ用の初期化

bool SProjectInit(void); // 西方Ｐｒｏｊｅｃｔ表示の初期化

bool GameExstgInit(void); // エキストラステージを始める

// NameRegistInit → inline wrapper in gameflow_manager.h
bool ScoreNameInit(void);                // お名前表示画面

bool GameNextStage(void); // 次のステージに移行する
