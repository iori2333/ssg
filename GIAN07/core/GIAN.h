/*                                                                           */
/*   GIAN.h   ゲーム全体の管理                                               */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

// 2000/02/23 : GIAN06 同等の性能をもつようになった。
// 2000/02/09 : 大幅な変更

// #define PBG_DEBUG		// デバッグモードを可能にする時に定義する

///// [Include Files] /////
#include "constants.h"

#include "enemy/BOSS.h"      // ボスの定義＆ボス用エフェクトなど
#include "enemy/boss_manager.h"   // BossManager + backward-compat wrappers
#include "enemy/enemy_manager.h" // EnemyManager + backward-compat wrappers
#include "effect/EFFECT.h"    // 主にテキストベースのエフェクト処理
#include "effect/EFFECT3D.h"  // ３Ｄエフェクト
#include "effect/FRAGMENT.h"  // 破片系エフェクト処理
#include "effect/effect_manager.h" // Effects 参照宣言
#include "gameflow/GAMEMAIN.h"  // メインのルーチン切り替え処理
#include "gameflow/gameflow_manager.h" // GameMain, DemoTimer, ... 参照宣言
#include "bullet/HOMINGL.h"   // ホーミングレーザーの処理
#include "bullet/LASER.h"     // 短いレーザー＆リフレクトレーザー処理
#include "bullet/LLASER.h"    // なが～いレーザーの処理
#include "bullet/bullet_manager.h" // BulletManager + backward-compat wrappers
#include "bullet/laser_manager.h"  // LaserManager + backward-compat wrappers
#include "LOADER.h"    // 各種ローダー
#include "player/MAID.h"      // その名の通り
#include "player/MAIDTAMA.h"  // 自機ショットの処理
#include "gameflow/PRankCtrl.h" // プレイランク管理
#include "stage/SCROLL.h"    // 背景スクロール＆ＳＣＬ管理

#include "game/ut_math.h" // for rnd()
#include "player/ITEM.h" // アイテム処理
#include "game_manager.h" // GameCount, GameStage, GameLevel, IsDemoplay

///// [ 定数 ] /////

// 座標関連 //
inline constexpr int X_MIN = 128; // 表示Ｘ座標最小値
inline constexpr int X_MAX = 511; // 表示Ｘ座標最大値
inline constexpr int X_MID = (X_MAX + X_MIN) / 2;
inline constexpr int Y_MIN = 0; // 表示Ｙ座標最小値
inline constexpr WINDOW_COORD Y_MAX = (GRP_RES.h - 1);
inline constexpr int Y_MID = (Y_MAX + Y_MIN) / 2;

inline constexpr WINDOW_LTRB PLAYFIELD_CLIP = {X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)};

inline constexpr int X_RNDV = -30000; // Ｘ座標のランダム指定用
inline constexpr int Y_RNDV = -30000; // Ｙ座標のランダム指定用

inline constexpr int GX_MIN = (X_MIN * 64);            // ゲーム座標上におけるＸ座標最小値
inline constexpr int GX_MAX = (X_MAX * 64);            // ゲーム座標上におけるＸ座標最大値
inline constexpr int GX_MID = (GX_MAX + GX_MIN) / 2; // ゲーム座標上におけるＸ座標中心値
inline constexpr int GY_MIN = (Y_MIN * 64);            // ゲーム座標上におけるＹ座標最小値
inline constexpr int GY_MAX = (Y_MAX * 64);            // ゲーム座標上におけるＹ座標最大値
inline constexpr int GY_MID = (GY_MAX + GY_MIN) / 2; // ゲーム座標上におけるＹ座標中心値

inline constexpr int SX_WID = (64 * 10);                         // サボテンのＸ幅？
inline constexpr int SY_WID = (64 * 10);                         // サボテンのＹ幅？
inline constexpr int SX_MIN = (GX_MIN + SX_WID);                 // サボテンのＸ座標最小値
inline constexpr int SX_MAX = (GX_MAX - SX_WID);                 // サボテンのＸ座標最大値
inline constexpr int SY_MIN = (GY_MIN + SY_WID + 30 * 64);       // サボテンのＹ座標最小値
inline constexpr int SY_MAX = (GY_MAX - SY_WID);                 // サボテンのＹ座標最大値
inline constexpr int SX_START = GX_MID;                        // サボテンの開始Ｘ座標
inline constexpr int SY_START = (GY_MAX + 180 * 64 /*- 50*64*/); // サボテンの開始Ｙ座標

inline constexpr int RL_WIDX = (32 - 4);                // 反射レーザー用_Ｘ座標_補正値
inline constexpr int RL_WIDY = 16;                      // 反射レーザー用_Ｙ座標_補正値
inline constexpr int RLX_MIN = (GX_MIN / 64 + RL_WIDX); // 反射レーザーの反射Ｘ座標最小値
inline constexpr int RLX_MAX = (GX_MAX / 64 - RL_WIDX); // 反射レーザーの反射Ｘ座標最大値
inline constexpr int RLY_MIN = (GY_MIN / 64 + RL_WIDY); // 反射レーザーの反射Ｙ座標最小値
inline constexpr int RLY_MAX = (GX_MAX / 64 - RL_WIDY); // 反射レーザーの反射Ｙ座標最大値

inline constexpr int NREG_SX = (X_MID - 13 * 9); // ネームレジスト用ウィンドウの開始Ｘ
inline constexpr int NREG_SY = (Y_MID + 100);    // ネームレジスト用ウィンドウの開始Ｙ
inline constexpr int NREGI_X = (X_MID - 8 * 7);            // ネームレジスト用ウィンドウ(名前表示部)の開始Ｘ
inline constexpr int NREGI_Y = (Y_MID + 60); // ネームレジスト用ウィンドウ(名前表示部)の開始Ｙ

inline constexpr int STG_RNDXY = 0; // 配置する座標がランダムの時の値...(なんだかよう分からん)

// スコア //
inline constexpr int SCORE_NAME = 9; // スコア用の最大文字列長(NULL含む)

///// [関数] /////
inline short SPEEDM(uint8_t v) { return static_cast<short>((v & 0x3f) << 4); } // 速度セット用
inline short WAVESP(uint8_t v) { return static_cast<short>(v << 4); }          // WAVE?用の速度セット

// Ｘ座標ランダム (requires runtime rnd(), so not constexpr)
inline int GX_RND() { return (X_MIN + rnd() % (X_MAX - X_MIN)) << 6; }
// Ｙ座標ランダム
inline int GY_RND() { return (Y_MIN + rnd() % (Y_MAX - Y_MIN)) << 6; }

// ヒットチェック: ヒットなら非ゼロ
inline bool HITCHK(int a, int b, int h) { return std::abs(a - b) < h; }

///// [構造体] /////

/*
// スコア管理用構造体 //
typedef struct tagSCORE_DATA{
        int64_t	score;
        uint8_t	weapon;
        char	name[SCORE_NAME];
} SCORE_DATA;

typedef struct tagHIGH_SCORE{
        SCORE_DATA		easy[8];
        SCORE_DATA		normal[8];
        SCORE_DATA		hard[8];
        SCORE_DATA		lunatic[8];
} HIGH_SCORE;
*/

///// [グローバル変数] /////
// extern HIGH_SCORE	*HighScore;
// extern char			ScoreTable[8][80];
// GameCount, GameStage, GameLevel, IsDemoplay → game_manager.h で参照として宣言

void StdStatusOutput(void);
