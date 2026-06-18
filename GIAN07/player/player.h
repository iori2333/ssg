///
/// Player - Player (maid) related logic
///

#pragma once

#include "game/cast.h"
#include "player_manager.h"
#include "player_types.h"
#include <cstdint>

// [ Constants ]

// Cactus/viv constants
inline constexpr int VIVDEAD_VAL = 300;   // Viv death time
inline constexpr int VIVMUTEKI_VAL = 180; // Viv invincibility time

inline constexpr int MAID_MOVE_DISABLE_TIME =
    (250 - 100); // Move-disabled duration

inline constexpr int BOMBMUTEKI_VAL = 60; // Bomb-end invincibility
inline constexpr int SBOPT_DX = 26;       // Option offset (not x64)

inline constexpr int DEATHBOMB_WINDOW =
    12; // Deathbomb input window (~192ms @62.5fps)

inline constexpr int EVADETIME_MAX = 256; // Max graze wait time

inline constexpr int SSP_WIDE = (64 * 9);
inline constexpr int SSP_HOMING = (64 * 9);
inline constexpr int SSP_LASER = (64 * 13);

// [ Player class ] moved to player_types.h

// [ Variables ]
// Accessed directly via Players.viv

// [ Backward-compatibility function wrappers (to be phased out) ]
inline void MaidDraw() { Players.viv.Draw(); }
inline void StateDraw() { Players.viv.DrawStatus(); }
inline void MaidMove() { Players.viv.Update(); }
inline void MaidSet() { Players.viv.Initialize(); }
inline void MaidNextStage() { Players.viv.PrepareNextStage(); }
inline void MaidDead() { Players.viv.OnDeath(); }
inline void MaidHit() { Players.viv.OnHit(); }
inline void evade_add(uint8_t n) { Players.viv.AddEvade(n); }
inline void evade_addEx(int x, int y, uint8_t n) {
  Players.viv.AddEvadeEx(x, y, n);
}
inline void score_add(int sc) { Players.viv.AddScore(sc); }
inline void WideBombDraw() { Players.viv.DrawWideBomb(); }
inline void PowerUp(uint8_t damage) { Players.viv.PowerUp(damage); }
inline uint8_t GetLaserDeg() { return Players.viv.GetLaserDeg(); }
// Laser angle calculation (public because MAIDTAMA.cpp references it)
uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);
