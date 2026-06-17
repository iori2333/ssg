/*
 *   Player type definition (separated to resolve circular dependency)
 *   Included by MAID.h and player_manager.h
 */

#pragma once

#include <cstdint>

///// [ Player クラス ] /////

struct Player {
  // --- 座標 ---
  int x, y;     // 現在の<表示>座標
  int vx, vy;   // オプションのズレ具合
  int opx, opy; // 現在のオプション基本座標

  // --- スコア ---
  int64_t score;  // 得点カウンタ
  int64_t dscore; // 得点増加値

  // --- かすり ---
  uint32_t evade_sum; // かすり合計
  int evadesc;        // かすり得点
  uint16_t evade;     // かすり回数
  uint16_t evade_c;   // 連続「かすり」の残り許容時間

  // --- ステータス ---
  char v;              // サボテンの移動速度基本値(後で64~45倍にする)
  uint8_t weapon;      // "とげ" の種類
  uint8_t exp;         // サボテンの経験値？
  uint8_t bomb;        // ボムの数
  uint8_t left;        // 残りサボテン数
  uint8_t credit;      // のこりクレジット
  uint16_t miss_count; // ミス回数
  uint16_t bomb_used;  // ボム使用回数

  uint8_t GrpID; // 表示すべきグラフィック

  // --- タイマー/状態 ---
  uint16_t bomb_time;   // ボムウェイト用
  uint16_t exp2;        // 経験値増加抑制用
  uint16_t muteki;      // 無敵フラグ(0:off !0:無敵時間カウンタ)
  uint16_t lay_time;    // レーザーの発射タイミング用
  uint8_t lay_grp;      // レーザーのグラフィック用
  uint8_t toge_time;    // "とげ" の発射タイミング用
  uint8_t toge_ex;      // とげ発射用特殊変数
  uint8_t ShiftCounter; // 押しっぱなし低速移動用

  bool bGameOver; // ゲームオーバー判定用フラグ
  bool BuzzSound; // かすった音を連続再生させないためのフラグ

  // --- メソッド ---
  void Draw();             // プレイヤー描画 (MaidDraw)
  void DrawStatus();       // 各種ステータス描画 (StateDraw)
  void Update();           // 毎フレーム更新 (MaidMove)
  void Initialize();       // 初期化 (MaidSet)
  void PrepareNextStage(); // 次のステージ準備 (MaidNextStage)
  void OnDeath();          // 死亡処理 (MaidDead)

  void AddEvade(uint8_t n); // かすりゲージ上昇 (evade_add)
  void AddEvadeEx(int x, int y,
                  uint8_t n);   // 指定座標からかすりエフェクト (evade_addEx)
  void AddScore(int sc);        // スコア加算 (score_add)
  void DrawWideBomb();          // ワイドボム描画 (WideBombDraw)
  void PowerUp(uint8_t damage); // パワーアップ処理
  uint8_t GetLaserDeg();        // レーザー角度取得
  uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
  uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);

private:
  void DrawLaserBomb(); // レーザーボム描画
  static uint8_t GetLeftOrRightLaserDeg(uint8_t LaserDeg, int i);
};

// 後方互換用エイリアス
// (MAID alias removed — use Player directly)
