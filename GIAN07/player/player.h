/*                                                                           */
/*   Maid.h   メイドさん関連の処理                                           */
/*                                                                           */
/*                                                                           */

#pragma once

#include "game/cast.h"
#include "player_types.h"
#include "player_manager.h"
#include <cstdint>

///// [ 定数 ] /////

// サボテン定数 //
inline constexpr int VIVDEAD_VAL   = 300; // びびっとの死亡時間．．．
inline constexpr int VIVMUTEKI_VAL = 180; // びびっとの無敵時間

inline constexpr int MAID_MOVE_DISABLE_TIME = (250 - 100); // 行動不能な時間

inline constexpr int BOMBMUTEKI_VAL = 60; // ボムの終端無敵時間
inline constexpr int SBOPT_DX       = 26; // オプションのずれ幅(x64ではない)

inline constexpr int EVADETIME_MAX = 256; // かすりマックス時の待ち時間

inline constexpr int SSP_WIDE   = (64 * 9); //
inline constexpr int SSP_HOMING = (64 * 9); //
inline constexpr int SSP_LASER  = (64 * 13); //

///// [ Player クラス ] → player_types.h に移動

///// [ 変数 ] /////
// Players.viv で直接アクセス

///// [ 後方互換用関数ラッパー（段階的に廃止予定）] /////
inline void MaidDraw()       { Players.viv.Draw(); }
inline void StateDraw()      { Players.viv.DrawStatus(); }
inline void MaidMove()       { Players.viv.Update(); }
inline void MaidSet()        { Players.viv.Initialize(); }
inline void MaidNextStage()  { Players.viv.PrepareNextStage(); }
inline void MaidDead()       { Players.viv.OnDeath(); }
inline void evade_add(uint8_t n)           { Players.viv.AddEvade(n); }
inline void evade_addEx(int x, int y, uint8_t n) { Players.viv.AddEvadeEx(x, y, n); }
inline void score_add(int sc)              { Players.viv.AddScore(sc); }
inline void WideBombDraw()                 { Players.viv.DrawWideBomb(); }
inline void PowerUp(uint8_t damage)        { Players.viv.PowerUp(damage); }
inline uint8_t GetLaserDeg()               { return Players.viv.GetLaserDeg(); }
// レーザー角度計算（MAIDTAMA.cpp から参照されるため public に公開）
uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);
