/*
 *   Ending.h   : エンディングの処理
 *
 */

#pragma once

/*  [更新履歴]
 *    Version 0.01 : 2000/12/20 :
 */

/***** [クラス定義] *****/

/***** [関数プロトタイプ] *****/
bool EndingInit(void);   // エンディングまわりの初期化
void EndingProc(bool &); // エンディング状態推移用プロシージャ
void EndingDraw(void);   // エンディング時の描画処理

/***** [グローバル変数] *****/
