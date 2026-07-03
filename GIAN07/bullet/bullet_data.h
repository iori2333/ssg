///
/// bullet_data - Core bullet/laser data structs & constants in namespace
/// `bullets::`.
///
/// All subsystem code operates on these.  The legacy unqualified aliases
/// (Bullet, LASER_DATA, ...) are re-exposed by the legacy-compat headers
/// for non-bullet callers via `using` declarations.
///

#pragma once

#include <array>
#include <cstdint>

#include "core/point.h"
#include "gfx/coords.h"
#include "gfx/graphics_backend.h"

struct EnemyData; // forward — full definition pulled in by translation units

namespace bullets {

// ============================================================
//  Constants
// ============================================================

inline constexpr auto TAMA_MAX = (801 * 3); // Maximum number of bullets
inline constexpr auto TAMA_EVADE = 1;       // Bullet graze value
inline constexpr auto TAMA1_POINT = 10000;  // Bullet score
inline constexpr auto TAMA2_POINT = 15000;  // Bullet score
inline constexpr auto TAMA_EVADE_RADIUS_SMALL = 24 * 64;
inline constexpr auto TAMA_EVADE_RADIUS_LARGE = 32 * 64;

inline constexpr auto TAMA_SMALL = 0x00;  // Upper 4 bits: small bullet
inline constexpr auto TAMA_LARGE = 0x10;  // Upper 4 bits: large bullet
inline constexpr auto TAMA_ANGLE = 0x20;  // Upper 4 bits: angle-specified
inline constexpr auto TAMA_EXTRA = 0x30;  // Upper 4 bits: extra type
inline constexpr auto TAMA_EXTRA2 = 0x40; // Upper 4 bits: "ofuda" type
inline constexpr auto TAMA_REN = 0x04;    // Rapid-fire attribute
inline constexpr auto TAMA_ZSET = 0x08;   // Cactus (player) set attribute
inline constexpr auto TAMASP_RND0 = 0x00;
inline constexpr auto TAMASP_RND1 = 0x40;
inline constexpr auto TAMASP_RND2 = 0x80;
inline constexpr auto TAMASP_RND3 = 0xc0;

inline constexpr auto TAMA_HIT_S = 3 * 64;
inline constexpr auto TAMA_HIT_M = 6 * 64;
inline constexpr auto TAMA_HIT_L = 9 * 64;
inline constexpr auto TAMA_HIT_XL = 12 * 64;

// Bullet type (lower 4 bits of `Bullet::type`)
inline constexpr auto T_NORM = 0x00;
inline constexpr auto T_NORM_A = 0x01;
inline constexpr auto T_HOMING = 0x02;
inline constexpr auto T_HOMING_M = 0x03;
inline constexpr auto T_ROLL = 0x04;
inline constexpr auto T_ROLL_A = 0x05;
inline constexpr auto T_ROLL_R = 0x06;
inline constexpr auto T_GRAVITY = 0x07;
inline constexpr auto T_CHANGE = 0x08;
inline constexpr auto T_SBHOMING = 0x09;
inline constexpr auto T_SBHBOMB = 0x0a;

// Bullet option (lower 4 bits of `Bullet::option`)
inline constexpr auto TOP_NONE = 0x00;
inline constexpr auto TOP_WAVE = 0x10;
inline constexpr auto TOP_ROLL = 0x20;
inline constexpr auto TOP_PURU = 0x30;
inline constexpr auto TOP_REFX = 0x40;
inline constexpr auto TOP_REFY = 0x50;
inline constexpr auto TOP_REFXY = 0x60;
inline constexpr auto TOP_DIV = 0x70;
inline constexpr auto TOP_BOMB = 0x80;

// Bullet command constants
inline constexpr auto TC_WAY = 0x00;
inline constexpr auto TC_ALL = 0x01;
inline constexpr auto TC_RND = 0x02;
inline constexpr auto TC_WAYS = 0x04;
inline constexpr auto TC_ALLS = 0x05;
inline constexpr auto TC_RNDS = 0x06;
inline constexpr auto TC_WAYZ = 0x08;
inline constexpr auto TC_ALLZ = 0x09;
inline constexpr auto TC_RNDZ = 0x0a;
inline constexpr auto TC_WAYSZ = 0x0c;
inline constexpr auto TC_ALLSZ = 0x0d;
inline constexpr auto TC_RNDSZ = 0x0e;

// Bullet effect (lower 4 bits usage currently being designed)
inline constexpr auto TE_NONE = 0x00;
inline constexpr auto TE_ROLL1 = 0x10;
inline constexpr auto TE_ROLL2 = 0x20;
inline constexpr auto TE_WARN = 0x30;
inline constexpr auto TE_ROCK = 0x40;
inline constexpr auto TE_CIRCLE1 = 0x50;
inline constexpr auto TE_CIRCLE2 = 0x60;
inline constexpr auto TE_DELETE = 0xf0;

// Bullet flags
inline constexpr auto TF_NONE = 0x00;
inline constexpr auto TF_CLIP = 0x01;
inline constexpr auto TF_EVADE = 0x02;
inline constexpr auto TF_DELETE = 0x80;

// ============================================================
//  Bullet command & data
// ============================================================

struct BulletCommand {
  int x, y;       // Bullet spawn position
  uint8_t d;      // Firing angle
  uint8_t dw;     // Firing spread width
  uint8_t n;      // Bullet count (fire in n directions)
  uint8_t ns;     // Rapid-fire count
  uint8_t v;      // Speed (lower 6 bits) & random element (upper 2 bits)
  uint8_t c;      // Bullet color & shape
  char a;         // Acceleration
  char vd;        // Angular velocity | homing rate
  uint8_t rep;    // Repeat count
  uint8_t cmd;    // Bullet command & effect
  uint8_t type;   // Bullet type
  uint8_t option; // Bullet attribute

  /// Fluent helpers (used by player weapon forms).  Default state is
  /// equivalent to the legacy `TamaSTDForm(c)`: cmd=TC_WAY, option=0,
  /// type=T_NORM.  Setters return *this for chaining.
  static BulletCommand way(uint8_t c) {
    BulletCommand self{};
    self.cmd = TC_WAY;
    self.option = TOP_NONE;
    self.type = T_NORM;
    self.c = c;
    return self;
  }
  BulletCommand &xy(int xx, int yy) {
    x = xx;
    y = yy;
    return *this;
  }
  BulletCommand &deg(int dd, int dww) {
    d = (uint8_t)dd;
    dw = (uint8_t)dww;
    return *this;
  }
  BulletCommand &spd(int vv, int aa) {
    v = (uint8_t)vv;
    a = (char)aa;
    return *this;
  }
  BulletCommand &num(int nn, int nss) {
    n = (uint8_t)nn;
    ns = (uint8_t)nss;
    return *this;
  }
  BulletCommand &vel_type(int t) {
    type = (uint8_t)t;
    return *this;
  }
  BulletCommand &homing(int repv, int vdv) {
    rep = (uint8_t)repv;
    vd = (char)vdv;
    return *this;
  }
};

struct Bullet {
  int x, y;       // Current display coordinates
  int tx, ty;     // Calculation coords (vibration effects)
  int vx, vy;     // Velocity components
  int v;          // Velocity
  int v0;         // Initial velocity
  char a;         // Acceleration
  double d;       // Direction angle (radians)
  int8_t vd;      // Angular velocity
  uint8_t c;      // Bullet color & shape
  uint8_t rep;    // Remaining control count
  uint8_t type;   // Bullet type
  uint8_t option; // Bullet attribute
  uint8_t effect; // Current effect
  uint16_t count; // Frame counter
  uint8_t flag;   // Deletion request flag
};

// ============================================================
//  Short / reflective laser
// ============================================================

inline constexpr auto LASER_MAX = 1000;

struct LaserCommand {
  int x, y; // Starting point
  int v;    // Initial velocity
  int w;    // Thickness (x64)
  int l;    // Final length (x64)
  int l2;   // Firing position offset (x64)
  uint8_t d;
  uint8_t dw;
  uint8_t n;
  uint8_t c;
  char a;
  uint8_t cmd;
  uint8_t type;
  uint8_t notr; // Reflector index to skip
};

inline constexpr auto LF_DELETE = 0x80;

struct LASER_DATA {
  int x, y;
  int vx, vy;
  int lx, ly; // Length offset (display)
  int wx, wy; // Width offset (display)
  int v;
  VERTEX_XY p[4];
  char a;
  double d;   // Direction angle (radians)
  int w, wmax;
  int l, lmax;
  int ltemp;
  uint16_t count;
  uint8_t c;
  uint8_t type;
  uint8_t flag;
  uint8_t notr;
  uint8_t evade;
};

// ============================================================
//  Long laser
// ============================================================

inline constexpr auto LLASER_MAX = 20;
inline constexpr auto LLASER_EVADE = 1;

inline constexpr auto LLS_LONG = 0x00;
inline constexpr auto LLS_LONGY = 0x01;
inline constexpr auto LLS_SETDEG = 0x02;
inline constexpr auto LLS_LONGZ = 0x03; // Player set

inline constexpr auto LLF_DISABLE = 0x00;
inline constexpr auto LLF_NORM = 0x01;
inline constexpr auto LLF_OPEN = 0x02;
inline constexpr auto LLF_CLOSE = 0x04;
inline constexpr auto LLF_CLOSEL = 0x08;
inline constexpr auto LLF_LINE = 0x10;

struct LongLaserCommand {
  EnemyData *e; // Emitting enemy
  int dx, dy;   // Launch offset
  int v;        // Speed
  int w;        // Final thickness
  uint8_t d;
  uint8_t c;
  uint8_t type;
};

struct LongLaserData {
  EnemyData *e;
  int x, y;
  int dx, dy;
  int lx, ly;
  int infx, infy;
  int wx, wy;
  int w, wmax;
  int v;
  uint32_t count;
  VERTEX_XY p[4];
  double d;   // Direction angle (radians)
  uint8_t c;
  uint8_t flag;
  uint8_t type;
  uint8_t EnemyID;
};

// ============================================================
//  Homing laser
// ============================================================

inline constexpr auto HLASER_MAX = 162;
inline constexpr auto HLASER_LEN = 7;
inline constexpr auto HLASER_SECTION = 4;

inline constexpr auto HL_NONE = 0;
inline constexpr auto HL_TYPE1 = 1;

inline constexpr auto HLS_NORM = 0x00;
inline constexpr auto HLS_CLEAR = 0x01;
inline constexpr auto HLS_DEAD = 0xff;

struct HomingLaserData {
  int Current;
  int v;
  int a;
  uint32_t Count;
  uint8_t Type;
  uint8_t State;
  uint8_t c;
  uint8_t Left;
  HomingLaserData *Next;
  DegPoint p[HLASER_LEN * HLASER_SECTION];
};

struct HomingLaserInfo {
  int x, y;
  uint8_t d;
  uint8_t dw;
  uint8_t n;
  uint8_t c;
  uint8_t type;
};

// ============================================================
//  Player shot pool sizing & damage
// ============================================================

inline constexpr auto MAIDTAMA_MAX = 200;

inline constexpr auto TID_WIDE_MAIN = 0x00;
inline constexpr auto TID_WIDE_SUB = 0x01;
inline constexpr auto TID_HOMING_MAIN = 0x02;
inline constexpr auto TID_HOMING_SUB = 0x03;
inline constexpr auto TID_LASER_MAIN = 0x04;
inline constexpr auto TID_LASER_SUB = 0x05;
inline constexpr auto TID_HOMING_BOMB_A = 0x06;
inline constexpr auto TID_HOMING_BOMB_B = 0x07;
inline constexpr auto TID_WIDE_FOCUS_MAIN = 0x08;
inline constexpr auto TID_WIDE_FOCUS_SUB = 0x09;
inline constexpr auto TID_HOMING_FOCUS_MAIN = 0x0a;
inline constexpr auto TID_HOMING_FOCUS_SUB = 0x0b;

inline constexpr auto TDM_WIDE_MAIN = 6;
inline constexpr auto TDM_WIDE_SUB = 4;
inline constexpr auto TDM_HOMING_MAIN = 6;
inline constexpr auto TDM_HOMING_SUB = 7;
inline constexpr auto TDM_LASER_MAIN = 2;
inline constexpr auto TDM_LASER_SUB = 5;
inline constexpr auto TDM_WIDE_FOCUS_MAIN = 6;
inline constexpr auto TDM_WIDE_FOCUS_SUB = 4;
inline constexpr auto TDM_HOMING_FOCUS_MAIN = 6;
inline constexpr auto TDM_HOMING_FOCUS_SUB = 7;

constexpr uint8_t TogeDamage[0x0c] = {
    TDM_WIDE_MAIN,
    TDM_WIDE_SUB,
    TDM_HOMING_MAIN,
    TDM_HOMING_SUB,
    TDM_LASER_MAIN,
    TDM_LASER_SUB,
    1,
    1,
    TDM_WIDE_FOCUS_MAIN,
    TDM_WIDE_FOCUS_SUB,
    TDM_HOMING_FOCUS_MAIN,
    TDM_HOMING_FOCUS_SUB,
};

// ============================================================
//  Public hit-radius helpers (kept as free functions for debug callers)
// ============================================================
int GetBulletHitRadius(uint8_t c);
int GetBulletEvadeRadius(uint8_t c);

} // namespace bullets