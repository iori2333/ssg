/*
 *   SCL.h — Stage Control Language command constants
 *
 *   Type-safe enum classes replacing the old inline constexpr uint8_t
 *   constants. Matches the pattern used by ECL (ecl_opcodes.h).
 */

#pragma once

#include <cstdint>

// ============================================================
// SCL opcodes
// ============================================================
enum class Scmd : uint8_t {
  TIME = 0x00,         // 次のイベントの発動時間
  ENEMY = 0x01,        // 敵イベント
  SSP = 0x02,          // スクロールスピードチェンジ
  EFC = 0x03,          // エフェクトセット
  END = 0x04,          // ＳＣＬ終了
  BOSS = 0x05,         // ボス発生
  MWOPEN = 0x06,       // メッセージウィンドウを開く
  MWCLOSE = 0x07,      // メッセージウィンドウを閉じる
  MSG = 0x08,          // メッセージを出力する
  KEY = 0x09,          // キー入力待ち
  NPG = 0x0a,          // 新しいページに変更する
  FACE = 0x0b,         // 顔を表示する
  MUSIC = 0x0c,        // 曲データをロードする
  BOSSDEAD = 0x0d,     // ボス強制破壊
  LOADFACE = 0x0e,     // 顔グラをロードする
  WAITEX = 0x0f,       // 条件待ち
  STAGECLEAR = 0x10,   // ステージクリア→次ステージへ
  MAPPALETTE = 0x11,   // パレットをマップパーツ用で初期化
  GAMECLEAR = 0x12,    // タイトルに戻る
  DELENEMY = 0x13,     // 敵を強制消去
  ENEMYPALETTE = 0x14, // 敵のパレットにする
  STAFF = 0x15,        // スタッフＩＤセット
  EXTRACLEAR = 0x16,   // エキストラステージクリア
};

// ============================================================
// EFC sub-commands (argument of Scmd::EFC)
// ============================================================
enum class Sefc : uint8_t {
  WARN = 0x00,       // ワーニング音・開始
  WARNSTOP = 0x01,   // ワーニング音・停止
  MUSICFADE = 0x02,  // 曲フェードアウト実行
  STG2BOSS = 0x03,   // ステージ２ボスのスクロール発動
  RASTERON = 0x04,   // ラスタースクロール開始
  RASTEROFF = 0x05,  // ラスタースクロール終了
  CFADEIN = 0x06,    // 円形フェードイン
  CFADEOUT = 0x07,   // 円形フェードアウト
  STG3BOSS = 0x08,   // ３面ボス雲
  STG3RESET = 0x09,  // ３面ボス雲リセット
  STG6CUBE = 0x0a,   // ６面ボス３Ｄキューヴ
  STG6RNDECL = 0x0b, // ６面ボス偽ＥＣＬ羅列
  STG4ROCK = 0x0c,   // ４面岩
  STG4LEAVE = 0x0d,  // ４面岩を画面外に吐き出す
  WHITEIN = 0x0e,    // ホワイトイン
  WHITEOUT = 0x0f,   // ホワイトアウト
  LOADEX01 = 0x10,   // エキストラボス１用画像をロード
  LOADEX02 = 0x11,   // エキストラボス２用画像をロード
  STG6RASTER = 0x12, // ６面ラスター
};

// ============================================================
// WAITEX condition codes (argument of Scmd::WAITEX)
// ============================================================
enum class Swait : uint8_t {
  BOSSLEFT = 0x00, // ボスの残り数
  BOSSHP = 0x01,   // ボスのＨＰ総和が指定値より小さい
};
