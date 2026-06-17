/*                                                                           */
/*   GameMain.h   ウィンドウシステム切り替えなどの処理                       */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

// 2000/02/03 : 製作開始

///// [Include Files] /////
#include "ending.h"
#include <functional>

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
[[nodiscard]] bool
GameInit(std::function<void(bool &)> next_proc); // ゲームの初期化をする
void GameRestart(); // ゲームを再開する(ESC 抜けから)
[[nodiscard]] bool GameExit(bool bNeedChgMusic = true); // ゲームから抜ける
void GameOverInit(); // ゲームオーバーの前処理
void GameContinue(); // コンティニューを行う場合

[[nodiscard]] bool
GameReplayInitAll(const char8_t *fn); // マルチステージリプレイ用の初期化

[[nodiscard]] bool SProjectInit(void); // 西方Ｐｒｏｊｅｃｔ表示の初期化

[[nodiscard]] bool GameExstgInit(void); // エキストラステージを始める

// NameRegistInit → inline wrapper in gameflow_manager.h
[[nodiscard]] bool ScoreNameInit(void); // お名前表示画面

[[nodiscard]] bool GameNextStage(void); // 次のステージに移行する
