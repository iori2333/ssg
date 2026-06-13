/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#pragma once

#include "ENEMY.h"
#include <array>
#include <cstdint>

///// [構造体] /////

// 特殊当たり判定 //
struct ExHitCheck {
  uint8_t flags[60][60];
};
using EXHITCHK = ExHitCheck;

// ボスデータ //
struct BossData {
  ENEMY_DATA Edat; // 標準の敵データ(実体であることに注意)
  EXHITCHK *Hit;   // 特殊当たり判定(NULL なら使用しない)

  void (*ExMove)(BossData *); // 特殊移動用関数

  uint32_t ExCount; // ある状態におけるカウンタ(推移時にゼロ初期化)
  uint8_t ExState;  // 特殊状態
  uint8_t IsUsed;   // このデータは使用されているか(非ゼロなら使用されている)
};
using BOSS_DATA = BossData;

///// [ 定数 ] /////
inline constexpr auto BOSS_MAX = 4; // ボスの最大出現数
inline constexpr auto BOSSHPG_HEIGHT = 24; // 体力ゲージの高さ

// ボスの体力ゲージ //
typedef struct tagBOSSHPG_INFO {
  uint32_t Now, Max; // 体力の現在値＆最大値
  uint32_t Next;     // 次の体力の値
  uint32_t Update;   // 更新用の値
  uint32_t Count;    // フレーム数保持

  uint16_t XTemp[BOSSHPG_HEIGHT]; // ＨＰゲージの演出用
  uint8_t State;                  // 状態
} BOSSHPG_INFO;

///// [ 関数 ] /////
void BossDataInit(
    void); // ボスデータ配列を初期化する(中断、ステージクリア時に使用)
void BossSet(int x, int y, uint32_t BossID);   // ボスをセットする(For SCL)
void BossSetEx(int x, int y, uint32_t BossID); // ボスをセットする(For ECL)
void BossMove(void);                           // ボスを動かす
void BossDraw(void);                           // ボスを描画する

void BossClearCmd(void);  // ボス用・敵弾クリアの前処理関数
int BossGetBitLeft(void); // 残りビット数を返す

void BossKillAll(void); // 現在出現しているボス全てのＨＰを０にする
bool BossDamage(int x, int y, int damage); // ボスにダメージを与える
bool BossDamage2(int x, int y,
                 int damage); // ボスにダメージを与える(ｙ上方向無限Ver)
void BossDamage3(int x, int y,
                 uint8_t d);  // ボスにダメージを与える(ナナメレーザー)
void BossDamage4(int damage); // ボスにダメージを与える(すべての敵)

void BossHPG_Draw(void); // ボスの体力ゲージを描画する

uint32_t GetBossHPSum(void); // ボスの体力の総和を求める

void BossINT(ENEMY_DATA *e, uint8_t IntID);        // ボス用割り込み処理
void BossBitAttack(ENEMY_DATA *e, uint32_t AtkID); // ビット攻撃アドレス指定
void BossBitLaser(ENEMY_DATA *e,
                  uint8_t LaserCmd); // ビットにレーザーコマンドセット
void BossBitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param); // ビット命令送信

///// [ 変数 ] /////
// Boss[], BossNow, BossHPG → boss_manager.cpp で参照として定義
extern std::array<BOSS_DATA, BOSS_MAX>& Boss;
extern uint16_t& BossNow;
extern BOSSHPG_INFO& BossHPG;
