///
/// Bullet - Definitions and various things related to bullets
///

#pragma once

#include "core/entity.h"

///// [Update history] /////

// -> From here, a bit old (1999...)
// Things that need to be changed, etc.
// > Eliminate ox, oy (TAMA.C also needs to be changed, troublesome...)
//
// > 5/17 (2:43): Eliminated ox, oy.
// Made automatic clipping depending on bullet type: related to the above
// Expanded .flag functionality (see constants below)
//
// > 6/13 (8:05): Dot-unit clipping

////Bullet constants////
inline constexpr auto TAMA_MAX = (801 * 3); // Maximum number of bullets
inline constexpr auto TAMA_EVADE = 1;       // Bullet graze value

inline constexpr auto TAMA1_POINT = 10000; // Bullet score
inline constexpr auto TAMA2_POINT = 15000; // Bullet score

inline constexpr auto TAMA_HITX = (2 * 64); // Bullet hitbox
inline constexpr auto TAMA_HITY = (4 * 64); // Bullet hitbox
inline constexpr auto TAMA_EVX_SMALL =
    ((8 + 8) * 64); // Small bullet graze detection (X)
inline constexpr auto TAMA_EVY_SMALL =
    ((16 + 8) * 64); // Small bullet graze detection (Y)
inline constexpr auto TAMA_EVX_LARGE =
    ((8 + 16) * 64); // Large bullet graze detection (X)
inline constexpr auto TAMA_EVY_LARGE =
    ((16 + 16) * 64); // Large bullet graze detection (Y)

inline constexpr auto TAMA_SMALL =
    0x00; // Upper 4 bits when bullet is small type
inline constexpr auto TAMA_LARGE =
    0x10; // Upper 4 bits when bullet is large type
inline constexpr auto TAMA_ANGLE =
    0x20; // Upper 4 bits when bullet is direction-specified type
inline constexpr auto TAMA_EXTRA =
    0x30; // Upper 4 bits when bullet is extra type
inline constexpr auto TAMA_EXTRA2 =
    0x40; // Upper 4 bits when bullet is "ofuda" type
inline constexpr auto TAMA_REN = 0x04;  // Bullet rapid-fire attribute
inline constexpr auto TAMA_ZSET = 0x08; // Bullet cactus (player) set attribute
inline constexpr auto TAMASP_RND0 = 0x00; // No speed random
inline constexpr auto TAMASP_RND1 = 0x40; // Speed random??
inline constexpr auto TAMASP_RND2 = 0x80; // Speed random??
inline constexpr auto TAMASP_RND3 = 0xc0; // Speed random??

////Bullet type constants (upper 4 bits currently unused)////
inline constexpr auto T_NORM = 0x00;   // Normal bullet: moves with (vx, vy)
inline constexpr auto T_NORM_A = 0x01; // Accelerating: rep acceleration count?
inline constexpr auto T_HOMING =
    0x02; // N-homing: rep homing count / a acceleration
inline constexpr auto T_HOMING_M =
    0x03; // N% homing: a acceleration / vd homing rate
inline constexpr auto T_ROLL =
    0x04; // Rolling: rep rotation time / vd angular velocity
inline constexpr auto T_ROLL_A =
    0x05; // Rolling (accelerating): above + a acceleration
inline constexpr auto T_ROLL_R = 0x06; // Rolling (reverse): same as above
inline constexpr auto T_GRAVITY =
    0x07; // Gravity: (vx, vy) & vy accelerated by (acceleration a)
inline constexpr auto T_CHANGE =
    0x08; // Angle change: changes angle to vd every rep frames
inline constexpr auto T_SBHOMING = 0x09; // Cactus homing
inline constexpr auto T_SBHBOMB = 0x0a;  // Cactus homing bomb

////Bullet option constants (lower 4 bits for option component spec)////
inline constexpr auto TOP_NONE = 0x00;  // No option
inline constexpr auto TOP_WAVE = 0x10;  // Wave: amplitude
inline constexpr auto TOP_ROLL = 0x20;  // Roll: rotation radius
inline constexpr auto TOP_PURU = 0x30;  // Vibrate: vibration intensity
inline constexpr auto TOP_REFX = 0x40;  // Reflect X: reflection count
inline constexpr auto TOP_REFY = 0x50;  // Reflect Y: reflection count
inline constexpr auto TOP_REFXY = 0x60; // Reflect XY: reflection count
inline constexpr auto TOP_DIV = 0x70;   // Split: bullet command on split
inline constexpr auto TOP_BOMB = 0x80;  // Bomb???: explosion radius

////Bullet command constants////
inline constexpr auto TC_WAY = 0x00;  // Fan-shaped fire
inline constexpr auto TC_ALL = 0x01;  // All-direction fire
inline constexpr auto TC_RND = 0x02;  // Random with base angle set
inline constexpr auto TC_WAYS = 0x04; // Fan-shaped fire & rapid fire
inline constexpr auto TC_ALLS = 0x05; // All-direction fire & rapid fire
inline constexpr auto TC_RNDS = 0x06; // Random with base angle set & rapid fire
inline constexpr auto TC_WAYZ = 0x08; // Fan-shaped fire & cactus set
inline constexpr auto TC_ALLZ = 0x09; // All-direction fire & cactus set
inline constexpr auto TC_RNDZ = 0x0a; // Random cactus set with base angle
inline constexpr auto TC_WAYSZ =
    0x0c; // Fan-shaped fire & rapid fire & cactus set
inline constexpr auto TC_ALLSZ =
    0x0d; // All-direction fire & rapid fire & cactus set
inline constexpr auto TC_RNDSZ =
    0x0e; // Random cactus set with base angle & rapid fire

////Bullet effect constants (lower 4 bits usage currently being designed!!)////
inline constexpr auto TE_NONE = 0x00;    // No effect
inline constexpr auto TE_ROLL1 = 0x10;   // Rolling charge effect
inline constexpr auto TE_ROLL2 = 0x20;   // Rolling charge effect
inline constexpr auto TE_WARN = 0x30;    // Warning display
inline constexpr auto TE_ROCK = 0x40;    // Lock-on
inline constexpr auto TE_CIRCLE1 = 0x50; // Ring effect (small -> large)
inline constexpr auto TE_CIRCLE2 = 0x60; // Ring effect (large -> small)
inline constexpr auto TE_DELETE = 0xf0;  // Delete effect

////Bullet flag constants////
inline constexpr auto TF_NONE = 0x00;   // Flag not set
inline constexpr auto TF_CLIP = 0x01;   // Do not delete even when off-screen
inline constexpr auto TF_EVADE = 0x02;  // Has grazed once
inline constexpr auto TF_DELETE = 0x80; // Delete this bullet

////Bullet command struct (improved safety)////
struct BulletCommand {
  int x, y; // Bullet spawn position

  uint8_t d;  // Firing angle
  uint8_t dw; // Firing spread width
  uint8_t n;  // Bullet count (fire in n directions)
  uint8_t ns; // Rapid-fire count (only valid when cmd s-bit is ON)
  uint8_t v;  // Speed (lower 6 bits) & random element (upper 2 bits)
  uint8_t c;  // Bullet color & shape
  char a;     // Acceleration (note: unit differs from speed)

  char vd; // Angular velocity | homing rate (cast to BYTE)

  uint8_t rep;    // Repeat count (rotation, n-homing, etc.)
  uint8_t cmd;    // Bullet command & effect
  uint8_t type;   // Bullet type
  uint8_t option; // Bullet attribute (vibration, reflection, burst, bomb)
};

////Bullet data struct////
struct Bullet {
  int x, y;   // Current display coordinates
  int tx, ty; // Calculation coordinates when using vibration effects
  int vx, vy; // Velocity (X, Y) components

  int v;  // Velocity
  int v0; // Initial velocity (used in rotation effects, etc.)
  char a; // Acceleration

  uint8_t d;    // Direction angle
  uint16_t d16; // Direction angle (fixed-point x256) -> used only in n% homing
  int8_t vd;    // Angular velocity

  uint8_t c; // Bullet color & shape

  uint8_t rep;    // Number of times to perform control by type
  uint8_t type;   // Bullet type (normal, accelerating, homing 2, rolling 3,
                  // gravity, change)
  uint8_t option; // Bullet attribute (vibration, reflection, burst, bomb)
  uint8_t effect; // Current effect (none, lock, ring, delete)

  uint16_t count; // Frame counter
  uint8_t flag;   // Bullet deletion request flag
};

// Backward compatibility aliases
// (TAMA_CMD alias removed — use BulletCommand directly)
// (TAMA_DATA alias removed — use Bullet directly)

////Various bullet variables////
// Access directly via Bullets.bullets, Bullets.command,
// Bullets.indices_small/large, Bullets.count_small/large

////Bullet functions////
// Implementation moved to BulletManager methods
// TamaSetForm, TamaSTDForm, TamaSetDeg, TamaSetNum, TamaSetSpd, TamaSetXY ->
// moved to bullet_manager.h

//// Graze (implementation in bullet.cpp where player.h is available) ////
void TamaEvadeAdd(Bullet *t);

// (Indsort<Bullet> wrapper removed — pass predicate directly)
