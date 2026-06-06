/*
 *   SCL command structures — strongly-typed parameter layouts for each
 *   Stage Control Language instruction.
 *
 *   Same pattern as ecl_commands.h: pure-data structs + tag-dispatched
 *   Decode() overloads + constexpr SclCmdLength().
 */

#pragma once

#include "ecl/SCL.h"
#include "game/endian.h"
#include <cstdint>

// --- Tag type for Decode dispatch ---
template <uint8_t Cmd> struct ScmdTag {};

// ===================================================================
// Command parameter structs (opcode byte NOT included)
// ===================================================================

struct SclCmdTime {
  uint32_t time;
};

struct SclCmdEnemy {
  int16_t x;
  int16_t y;
  uint8_t id;
};

struct SclCmdBoss {
  int16_t x;
  int16_t y;
  uint8_t id;
};

struct SclCmdBossdead {}; // op only
struct SclCmdMwopen {};   // op only
struct SclCmdMwclose {};  // op only

struct SclCmdMsg {
  // String follows opcode byte; length computed at runtime via strlen
  // No fixed-size struct — handled as special case in Execute()
};

struct SclCmdFace {
  uint8_t face_id;
};

struct SclCmdLoadface {
  uint8_t surf_id;
  uint8_t file_no;
};

struct SclCmdNpg {}; // op only
struct SclCmdEnd {}; // op only
struct SclCmdKey {}; // op only

struct SclCmdSsp {
  int16_t speed;
};

struct SclCmdMusic {
  uint8_t track;
};

struct SclCmdDelenemy {}; // op only

struct SclCmdEfc {
  uint8_t efc_id;
};

struct SclCmdWaitex {
  uint8_t cond;
  uint32_t value;
};

struct SclCmdStageclear {};   // op only
struct SclCmdGameclear {};    // op only
struct SclCmdExtraclear {};   // op only
struct SclCmdMappalette {};   // op only
struct SclCmdEnemypalette {}; // op only

// ===================================================================
// SclCmdLength — instruction length lookup
// ===================================================================

inline constexpr uint8_t SclCmdLength(uint8_t cmd) {
  switch (cmd) {
  case SCL_TIME:
    return 5;
  case SCL_ENEMY:
    return 6;
  case SCL_BOSS:
    return 6;
  case SCL_BOSSDEAD:
    return 1;
  case SCL_MWOPEN:
    return 1;
  case SCL_MWCLOSE:
    return 1;
  case SCL_MSG:
    return 0; // variable length, handled specially
  case SCL_FACE:
    return 2;
  case SCL_LOADFACE:
    return 3;
  case SCL_NPG:
    return 1;
  case SCL_END:
    return 1;
  case SCL_KEY:
    return 1;
  case SCL_SSP:
    return 3;
  case SCL_MUSIC:
    return 2;
  case SCL_DELENEMY:
    return 1;
  case SCL_EFC:
    return 2;
  case SCL_WAITEX:
    return 6;
  case SCL_STAGECLEAR:
    return 1;
  case SCL_GAMECLEAR:
    return 1;
  case SCL_EXTRACLEAR:
    return 1;
  case SCL_MAPPALETTE:
    return 1;
  case SCL_ENEMYPALETTE:
    return 1;
  default:
    return 1;
  }
}

// ===================================================================
// Decode — tag-dispatched bytecode → typed command struct
// ===================================================================

inline SclCmdTime Decode(ScmdTag<SCL_TIME>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline SclCmdEnemy Decode(ScmdTag<SCL_ENEMY>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), raw[5]};
}

inline SclCmdBoss Decode(ScmdTag<SCL_BOSS>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), raw[5]};
}

inline SclCmdFace Decode(ScmdTag<SCL_FACE>, const uint8_t *raw) {
  return {raw[1]};
}

inline SclCmdLoadface Decode(ScmdTag<SCL_LOADFACE>, const uint8_t *raw) {
  return {raw[1], raw[2]};
}

inline SclCmdSsp Decode(ScmdTag<SCL_SSP>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1]))};
}

inline SclCmdMusic Decode(ScmdTag<SCL_MUSIC>, const uint8_t *raw) {
  return {raw[1]};
}

inline SclCmdEfc Decode(ScmdTag<SCL_EFC>, const uint8_t *raw) {
  return {raw[1]};
}

inline SclCmdWaitex Decode(ScmdTag<SCL_WAITEX>, const uint8_t *raw) {
  return {raw[1], U32LEAt(&raw[2])};
}

// Single-byte (no-param) commands all decode to empty struct
inline SclCmdEnd Decode(ScmdTag<SCL_BOSSDEAD>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_MWOPEN>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_MWCLOSE>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_NPG>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_END>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_KEY>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_DELENEMY>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_STAGECLEAR>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_GAMECLEAR>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_EXTRACLEAR>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_MAPPALETTE>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<SCL_ENEMYPALETTE>, const uint8_t *) {
  return {};
}
