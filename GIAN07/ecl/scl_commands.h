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
template <Scmd Cmd> struct ScmdTag {};

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
  Sefc efc_id;
};

struct SclCmdWaitex {
  Swait cond;
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

inline constexpr uint8_t SclCmdLength(Scmd cmd) {
  switch (cmd) {
  case Scmd::TIME:
    return 5;
  case Scmd::ENEMY:
    return 6;
  case Scmd::BOSS:
    return 6;
  case Scmd::BOSSDEAD:
    return 1;
  case Scmd::MWOPEN:
    return 1;
  case Scmd::MWCLOSE:
    return 1;
  case Scmd::MSG:
    return 0; // variable length, handled specially
  case Scmd::FACE:
    return 2;
  case Scmd::LOADFACE:
    return 3;
  case Scmd::NPG:
    return 1;
  case Scmd::END:
    return 1;
  case Scmd::KEY:
    return 1;
  case Scmd::SSP:
    return 3;
  case Scmd::MUSIC:
    return 2;
  case Scmd::DELENEMY:
    return 1;
  case Scmd::EFC:
    return 2;
  case Scmd::WAITEX:
    return 6;
  case Scmd::STAGECLEAR:
    return 1;
  case Scmd::GAMECLEAR:
    return 1;
  case Scmd::EXTRACLEAR:
    return 1;
  case Scmd::MAPPALETTE:
    return 1;
  case Scmd::ENEMYPALETTE:
    return 1;
  default:
    return 1;
  }
}

// ===================================================================
// Decode — tag-dispatched bytecode → typed command struct
// ===================================================================

inline SclCmdTime Decode(ScmdTag<Scmd::TIME>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline SclCmdEnemy Decode(ScmdTag<Scmd::ENEMY>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), raw[5]};
}

inline SclCmdBoss Decode(ScmdTag<Scmd::BOSS>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), raw[5]};
}

inline SclCmdFace Decode(ScmdTag<Scmd::FACE>, const uint8_t *raw) {
  return {raw[1]};
}

inline SclCmdLoadface Decode(ScmdTag<Scmd::LOADFACE>, const uint8_t *raw) {
  return {raw[1], raw[2]};
}

inline SclCmdSsp Decode(ScmdTag<Scmd::SSP>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1]))};
}

inline SclCmdMusic Decode(ScmdTag<Scmd::MUSIC>, const uint8_t *raw) {
  return {raw[1]};
}

inline SclCmdEfc Decode(ScmdTag<Scmd::EFC>, const uint8_t *raw) {
  return {static_cast<Sefc>(raw[1])};
}

inline SclCmdWaitex Decode(ScmdTag<Scmd::WAITEX>, const uint8_t *raw) {
  return {static_cast<Swait>(raw[1]), U32LEAt(&raw[2])};
}

// Single-byte (no-param) commands all decode to empty struct
inline SclCmdEnd Decode(ScmdTag<Scmd::BOSSDEAD>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::MWOPEN>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::MWCLOSE>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::NPG>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::END>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::KEY>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::DELENEMY>, const uint8_t *) { return {}; }
inline SclCmdEnd Decode(ScmdTag<Scmd::STAGECLEAR>, const uint8_t *) {
  return {};
}
inline SclCmdEnd Decode(ScmdTag<Scmd::GAMECLEAR>, const uint8_t *) {
  return {};
}
inline SclCmdEnd Decode(ScmdTag<Scmd::EXTRACLEAR>, const uint8_t *) {
  return {};
}
inline SclCmdEnd Decode(ScmdTag<Scmd::MAPPALETTE>, const uint8_t *) {
  return {};
}
inline SclCmdEnd Decode(ScmdTag<Scmd::ENEMYPALETTE>, const uint8_t *) {
  return {};
}
