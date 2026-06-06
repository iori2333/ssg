/*                                                                           */
/*   EFFECT.h   エフェクト管理                                               */
/*                                                                           */
/*                                                                           */

#pragma once

#include "game/narrow.h"
#include <cstdint>

///// [更新履歴] /////
// 2000/04/28 : 円エフェクトを作成
// 2000/04/15 : 円形フェード系関数の追加
// 2000/02/23 : 開発開始(->Ver0.20)

///// [ 定数 ] /////
inline constexpr auto SEFFECT_MAX = 1000;
inline constexpr auto LOCKON_MAX = 2; // 最大ロック数
inline constexpr auto CIRCLE_EFC_MAX = 10; // 円エフェクト最大数

inline constexpr auto CEFC_NONE = 0x00; // CircleEffect未使用
inline constexpr auto CEFC_STAR = 0x01; // お星様系エフェクト
inline constexpr auto CEFC_CIRCLE1 = 0x02; // 集まる円エフェクト
inline constexpr auto CEFC_CIRCLE2 = 0x03; // 離れる円エフェクト

inline constexpr auto SEFC_NONE = 0x00; // 未使用
inline constexpr auto SEFC_STR1 = 0x01; // 文字列エフェクト１
inline constexpr auto SEFC_STR1_2 = 0x02; // 文字列一時停止
inline constexpr auto SEFC_STR1_3 = 0x03; // 文字列爆発

inline constexpr auto SEFC_MTITLE1 = 0x04; // 曲名表示エフェクト(出動)
inline constexpr auto SEFC_MTITLE2 = 0x05; // 曲名表示エフェクト(停止)
inline constexpr auto SEFC_MTITLE3 = 0x06; // 曲名表示エフェクト(退却)

inline constexpr auto SEFC_GAMEOVER = 0x07; // ワーニングの表示とか
inline constexpr auto SEFC_GAMEOVER2 = 0x08; // ワーニングの表示とか

inline constexpr auto SEFC_STR2 = 0x10; // 得点アイテム用？エフェクト

inline constexpr auto LOCKON_NONE = 0x00; // ロックしていない
inline constexpr auto LOCKON_01 = 0x01; // ロックオン開始
inline constexpr auto LOCKON_02 = 0x02; // ロックオン停止
inline constexpr auto LOCKON_03 = 0x03; // ロックオン解放？

inline constexpr auto SCNEFC_NONE = 0x00; // エフェクト無し
inline constexpr auto SCNEFC_CFADEIN = 0x01; // 円形フェードイン
inline constexpr auto SCNEFC_CFADEOUT = 0x02; // 円形フェードアウト
inline constexpr auto SCNEFC_WHITEIN = 0x03; // ホワイトイン
inline constexpr auto SCNEFC_WHITEOUT = 0x04; // ホワイトアウト

///// [構造体] /////
struct CircleEffectData {
  int x, y;       // 中心座標
  int r, rmax;    // 半径／最大半径
  uint32_t count; // カウンタ
  uint8_t type;   // 円エフェクトの種類
                  //	uint8_t	Level;	// 円エフェクトの段階
  uint8_t d;      // 円エフェクトの角度(謎)
};
using CIRCLE_EFC_DATA = CircleEffectData;

struct StringEffectData {
  int x, y;
  int vx, vy;

  uint32_t time;
  uint32_t point;

  uint8_t cmd;
  char c;
};
using SEFFECT_DATA = StringEffectData;

struct LockOnInfo {
  int *x, *y;        // ロック座標へのポインタ
  int width, height; // 幅＆高さ
  int vx, vy;        // 速度成分
  uint32_t count;    // カウンタ
  uint8_t state;     // 状態
};
using LOCKON_INFO = LockOnInfo;

struct ScreenEffectState {
  uint8_t cmd;    // 発動中エフェクト
  uint32_t count; // エフェクトに対するカウンタ
};
using SCREENEFC_INFO = ScreenEffectState;

///// [ 関数 ] /////
void MTitleInit(void);  // Registers the music title rectangle
void SEffectInit(void); // エフェクトの初期化を行う
void StringEffect(int x, int y,
                  const char *s); // 文字列系エフェクト(上に表示する奴)
void StringEffect2(int x, int y, uint32_t point); // 得点表示エフェクト
void StringEffect3(void);                         // ゲームオーバーの表示
void SetMusicTitle(int y, Narrow::string_view s); // 曲名の表示

void SEffectMove(void); // エフェクトを動かす(仕様変更の可能性があります)
void SEffectDraw(void); // エフェクトを描画する(仕様変更の可能性があります)

void CEffectInit(void);                      // 円エフェクトの初期化
void CEffectMove(void);                      // 円エフェクトを動かす
void CEffectDraw(void);                      // 円エフェクトを描画する
void CEffectSet(int x, int y, uint8_t type); // 円エフェクトをセットする

void ScreenEffectInit(void);       // 画面全体に対するエフェクトの初期化
void ScreenEffectSet(uint8_t cmd); // 画面全体に対するエフェクトをセットする
void ScreenEffectMove(void);       // 画面全体に対するエフェクトを動かす
void ScreenEffectDraw(void);       // 画面全体に対するエフェクトを描画する

void WarningEffectInit(void); // ワーニングの初期化
void WarningEffectSet(void);  // ワーニング発動！！
void WarningEffectMove(void); // ワーニングの動作
void WarningEffectDraw(void); // ワーニングの描画

void ObjectLockOnInit(void);                           // ロックオン配列を初期化
void ObjectLockOn(int *x, int *y, int wx64, int hx64); // 何かをロックオンする
void ObjectLockMove(void); // ロックオンアニメーション動作
void ObjectLockDraw(void); // ロックオン枠描画

void GrpDrawSpect(int x, int y); // スペアナ描画
void GrpDrawNote(void);          // 押されているところを表示

///// [ 変数 ] /////
extern SEFFECT_DATA SEffect[SEFFECT_MAX];
extern CIRCLE_EFC_DATA CEffect[CIRCLE_EFC_MAX];
extern LOCKON_INFO LockInfo[LOCKON_MAX];
extern SCREENEFC_INFO ScreenInfo;

