/*
 *   ECL command structures — strongly-typed representations of each bytecode
 *   instruction. Each struct models the parameter layout of one ECL opcode.
 *
 *   Decode() overloads (tag-dispatched via EclOpTag<Op>) handle endian
 *   conversion and byte offset calculation in one place.
 *
 *   EclCmdLength(op) returns the total byte length of an instruction (opcode
 *   byte + parameters), replacing the old ECL_CmdLen[] table lookup.
 */

#pragma once

#include <cstdint>

#include "ecl/ecl_opcodes.h"
#include "game/endian.h"

// --- Tag type for Decode dispatch ---
template <EclOp Op> struct EclOpTag {};

// ===================================================================
// Command parameter structs (opcode byte NOT included)
// ===================================================================

// -- 0x0?: Control flow --

struct CmdSetup {
  uint32_t hp;
  uint32_t score;
};

struct CmdEnd {}; // no params, just kills the enemy

struct CmdJmp {
  uint32_t target;
}; // unconditional jump

struct CmdLoop {
  uint32_t target;
  uint16_t count; // iteration count + 1
};

struct CmdCall {
  uint32_t target;
}; // subroutine call

struct CmdRet {}; // return from subroutine

struct CmdJhpl {
  uint32_t target;
  uint32_t value;
}; // jump if HP > value

struct CmdJhps {
  uint32_t target;
  uint32_t value;
}; // jump if HP < value

struct CmdJdif {
  uint32_t t_easy;
  uint32_t t_norm;
  uint32_t t_hard;
  uint32_t t_luna;
}; // difficulty-based jump table

struct CmdJdsb {
  uint32_t target;
}; // jump if facing player

struct CmdJfcl {
  uint32_t target;
  uint32_t value;
}; // jump if frame count > value

struct CmdJfcs {
  uint32_t target;
  uint32_t value;
}; // jump if frame count < value

struct CmdSti {
  uint32_t addr;
  EclIntVec cond;
  uint32_t value;
}; // set interrupt vector

struct CmdCli {
  EclIntVec vec;
}; // clear interrupt vector

// -- 0x1?: Movement --

struct CmdNop {
  uint16_t frames;
}; // wait N frames

struct CmdNopsc {
  uint16_t frames;
}; // wait N frames, drift with scroll

struct CmdMov {
  uint16_t frames;
}; // linear movement

struct CmdRol {
  int8_t deg_delta;
  uint16_t frames;
}; // rotational movement

struct CmdLrol {
  int32_t vx;
  int32_t vy;
  int8_t deg_delta;
  uint16_t frames;
}; // linear + rotational

struct CmdWavx {
  int32_t vx;
  uint8_t amp;
  int8_t deg_delta;
  uint16_t frames;
}; // wave-X movement

struct CmdWavy {
  int32_t vy;
  uint8_t amp;
  int8_t deg_delta;
  uint16_t frames;
}; // wave-Y movement

struct CmdMxa {
  uint16_t target_x;
  uint16_t frames;
}; // absolute X movement

struct CmdMya {
  uint16_t target_y;
  uint16_t frames;
}; // absolute Y movement

struct CmdMxya {
  uint16_t target_x;
  uint16_t target_y;
  uint16_t frames;
}; // absolute XY movement

struct CmdMxs {
  uint16_t frames;
}; // X toward player

struct CmdMys {
  uint16_t frames;
}; // Y toward player

struct CmdMxys {
  uint16_t frames;
}; // XY toward player

struct CmdAcc {
  int8_t accel;
  uint16_t frames;
}; // accelerate/decelerate

struct CmdAccxya {}; // unimplemented

struct CmdGrax {
  int8_t gravity;
}; // gravity + X-bounce

// -- 0x2?: Value assignment --

struct CmdDega {
  uint8_t deg;
}; // set angle absolute

struct CmdDegr {
  int8_t delta;
}; // set angle relative

struct CmdDegx {};  // set angle random
struct CmdDegxu {}; // set angle random (up)
struct CmdDegxd {}; // set angle random (down)
struct CmdDegex {}; // set angle special
struct CmdDegs {};  // set angle toward player
struct CmdDegx2 {}; // set angle random (bounded)
struct CmdXys {};   // set position to player
struct CmdXyrnd {}; // set position random

struct CmdSpda {
  int32_t v;
}; // set speed absolute

struct CmdSpdr {
  int32_t delta;
}; // set speed relative

struct CmdXya {
  uint16_t x;
  uint16_t y;
}; // set position absolute (pixel coords)

struct CmdXyr {
  int16_t dx;
  int16_t dy;
}; // set position relative (pixel coords)

struct CmdXyl {
  uint16_t length;
}; // set position relative (polar)

// -- 0x4?: Bullet firing --

struct CmdTama {};   // fire bullets (standard)
struct CmdTama2 {};  // fire bullets (no difficulty scaling)
struct CmdTamal {};  // fire bullets (line pattern)
struct CmdTamaex {}; // fire bullets (extra boss pattern)
struct CmdTclr {};   // clear all bullets
struct CmdT2item {
  uint8_t pct;
}; // convert bullets to items

struct CmdTauto {
  uint8_t interval;
}; // set auto-fire interval

struct CmdTxyr {
  int16_t x;
  int16_t y;
}; // set bullet spawn offset

struct CmdTcmd {
  uint8_t cmd;
}; // set bullet command type

struct CmdTdega {
  uint8_t d;
  uint8_t dw;
}; // set bullet angle absolute

struct CmdTdegr {
  int8_t dd;
  int8_t ddw;
}; // set bullet angle relative

struct CmdTdegs {}; // set bullet angle toward player
struct CmdTdege {}; // set bullet angle to own direction

struct CmdTnuma {
  uint8_t n;
  uint8_t ns;
}; // set bullet count absolute

struct CmdTnumr {
  int8_t dn;
  int8_t dns;
}; // set bullet count relative

struct CmdTspda {
  uint8_t v;
  int8_t a;
}; // set bullet speed absolute

struct CmdTspdr {
  int8_t dv;
  int8_t da;
}; // set bullet speed relative

struct CmdTopt {
  uint8_t option;
}; // set bullet option

struct CmdTtype {
  uint8_t type;
}; // set bullet type

struct CmdTcol {
  uint8_t color;
}; // set bullet color

struct CmdTvdeg {
  int8_t vd;
}; // set bullet angular velocity

struct CmdTrep {
  uint8_t rep;
}; // set bullet repeat count

// -- 0x6?: Laser firing --

struct CmdLaser {};  // fire laser (standard)
struct CmdLaser2 {}; // fire laser (variant)

struct CmdLcmd {
  uint8_t cmd;
}; // set laser command type

struct CmdLla {
  int32_t length;
}; // set laser length absolute

struct CmdLlr {
  int32_t delta;
}; // set laser length relative

struct CmdLl2 {
  int32_t offset;
}; // set laser spawn offset

struct CmdLdega {
  uint8_t d;
  uint8_t dw;
}; // set laser angle absolute

struct CmdLdegr {
  int8_t dd;
  int8_t ddw;
}; // set laser angle relative

struct CmdLnumpa {
  uint8_t n;
}; // set laser count absolute
struct CmdLnumr {
  int8_t dn;
}; // set laser count relative

struct CmdLspda {
  int32_t v;
}; // set laser speed absolute

struct CmdLspdr {
  int32_t dv;
}; // set laser speed relative

struct CmdLcol {
  uint8_t color;
}; // set laser color

struct CmdLtype {
  uint8_t type;
}; // set laser type

struct CmdLwa {
  int32_t width;
}; // set laser width absolute

struct CmdLdegs {}; // set laser angle toward player
struct CmdLdege {}; // set laser angle to own direction

struct CmdLxy {
  int16_t x;
  int16_t y;
}; // set laser spawn position

// -- 0x8?: Large laser & homing --

struct CmdLlset {}; // set up large laser
struct CmdLlopen {
  uint8_t id;
}; // open large laser
struct CmdLlclose {
  uint8_t id;
}; // close large laser
struct CmdLlclosel {
  uint8_t id;
}; // close large laser to line
struct CmdLldegr {
  int8_t deg;
  uint8_t id;
}; // rotate large laser
struct CmdHlaser {}; // fire homing laser

// -- 0x9?: Flag toggles --
// All no-param: toggle enemy flags
struct CmdDrawOn {};
struct CmdDrawOff {};
struct CmdClipOn {};
struct CmdClipOff {};
struct CmdDamageOn {};
struct CmdDamageOff {};
struct CmdHitsbOn {};
struct CmdHitsbOff {};
struct CmdRlchgOn {};
struct CmdRlchgOff {};

// -- 0xA?: Special commands --

struct CmdAnm {
  uint8_t ptn;
  int8_t sp;
}; // set animation

struct CmdAnmex {
  uint8_t ptn;
}; // set damage animation

struct CmdPse {
  uint8_t id;
}; // play sound effect

struct CmdInt {
  EclIntType id;
}; // boss interrupt

struct CmdExdegd {
  uint8_t d;
}; // set special angle increment

struct CmdEnemyset {
  int16_t x;
  int16_t y;
  uint8_t ecl_id;
}; // spawn enemy

struct CmdEnemysetd {
  int16_t x;
  int16_t y;
  EclReg reg;
  uint8_t ecl_id;
}; // spawn enemy with angle

struct CmdHitxy {
  uint16_t w;
  uint16_t h;
}; // set hitbox size

struct CmdItem {
  uint8_t type;
}; // set drop item type

struct CmdStg4efc {
  uint8_t cmd;
}; // stage 4 boss effect sync

struct CmdStg3efc {}; // stage 3 star effect

struct CmdBitlaser {
  uint8_t cmd;
}; // bit laser command

struct CmdBitattack {
  uint32_t atk_id;
}; // bit attack pattern

struct CmdBitcmd {
  uint8_t cmd;
  int32_t param;
}; // bit command

struct CmdBossset {
  uint8_t id;
}; // spawn boss

struct CmdCefc {
  int16_t x;
  int16_t y;
  uint8_t id;
}; // circle effect

// -- 0xB?: Register operations --

struct CmdMovr {
  EclReg dst;
  EclReg src;
}; // move register

struct CmdMovc {
  EclReg dst;
  uint32_t value;
}; // move constant to register

struct CmdAdd {
  EclReg dst;
  EclReg src;
}; // add to register

struct CmdSub {
  EclReg dst;
  EclReg src;
}; // subtract from register

struct CmdSinl {
  EclReg dst;
  EclReg src;
}; // sinl to register

struct CmdCosl {
  EclReg dst;
  EclReg src;
}; // cosl to register

struct CmdMod {
  EclReg dst;
  uint32_t value;
}; // modulo register

struct CmdRnd {
  EclReg dst;
}; // random to register

struct CmdCmpr {
  EclReg reg0;
  EclReg reg1;
}; // compare registers

struct CmdCmpc {
  EclReg reg;
  uint32_t value;
}; // compare register with constant

struct CmdInc {
  EclReg dst;
}; // increment register

struct CmdDec {
  EclReg dst;
}; // decrement register

struct CmdJl {
  uint32_t target;
}; // jump if greater

struct CmdJs {
  uint32_t target;
}; // jump if less

struct CmdJeq {
  uint32_t target;
}; // jump if equal

// ===================================================================
// EclCmdLength — constexpr instruction length lookup (replaces ECL_CmdLen[])
// ===================================================================

constexpr uint8_t EclCmdLength(EclOp op) {
  switch (op) {
  // 0x0?: Control flow
  case EclOp::SETUP:
    return 9;
  case EclOp::END:
    return 1;
  case EclOp::JMP:
    return 5;
  case EclOp::LOOP:
    return 7;
  case EclOp::CALL:
    return 5;
  case EclOp::RET:
    return 1;
  case EclOp::JHPL:
    return 9;
  case EclOp::JHPS:
    return 9;
  case EclOp::JDIF:
    return 17;
  case EclOp::JDSB:
    return 5;
  case EclOp::JFCL:
    return 9;
  case EclOp::JFCS:
    return 9;
  case EclOp::STI:
    return 10;
  case EclOp::CLI:
    return 2;
  // 0x1?: Movement
  case EclOp::NOP:
    return 3;
  case EclOp::NOPSC:
    return 3;
  case EclOp::MOV:
    return 3;
  case EclOp::ROL:
    return 4;
  case EclOp::LROL:
    return 12;
  case EclOp::WAVX:
    return 9;
  case EclOp::WAVY:
    return 9;
  case EclOp::MXA:
    return 5;
  case EclOp::MYA:
    return 5;
  case EclOp::MXYA:
    return 7;
  case EclOp::MXS:
    return 3;
  case EclOp::MYS:
    return 3;
  case EclOp::MXYS:
    return 3;
  case EclOp::ACC:
    return 4;
  case EclOp::ACCXYA:
    return 7;
  case EclOp::GRAX:
    return 2;
  // 0x2?: Value assignment
  case EclOp::DEGA:
    return 2;
  case EclOp::DEGR:
    return 2;
  case EclOp::DEGX:
    return 1;
  case EclOp::DEGS:
    return 1;
  case EclOp::SPDA:
    return 5;
  case EclOp::SPDR:
    return 5;
  case EclOp::XYA:
    return 5;
  case EclOp::XYR:
    return 5;
  case EclOp::DEGXU:
    return 1;
  case EclOp::DEGXD:
    return 1;
  case EclOp::DEGEX:
    return 1;
  case EclOp::XYS:
    return 1;
  case EclOp::DEGX2:
    return 1;
  case EclOp::XYRND:
    return 1;
  case EclOp::XYL:
    return 3;
  // 0x4?: Bullet firing
  case EclOp::TAMA:
    return 1;
  case EclOp::TAUTO:
    return 2;
  case EclOp::TXYR:
    return 5;
  case EclOp::TCMD:
    return 2;
  case EclOp::TDEGA:
    return 3;
  case EclOp::TDEGR:
    return 3;
  case EclOp::TNUMA:
    return 3;
  case EclOp::TNUMR:
    return 3;
  case EclOp::TSPDA:
    return 3;
  case EclOp::TSPDR:
    return 3;
  case EclOp::TOPT:
    return 2;
  case EclOp::TTYPE:
    return 2;
  case EclOp::TCOL:
    return 2;
  case EclOp::TVDEG:
    return 2;
  case EclOp::TREP:
    return 2;
  case EclOp::TDEGS:
    return 1;
  case EclOp::TDEGE:
    return 1;
  case EclOp::TAMA2:
    return 1;
  case EclOp::TCLR:
    return 1;
  case EclOp::TAMAL:
    return 1;
  case EclOp::T2ITEM:
    return 2;
  case EclOp::TAMAEX:
    return 1;
  // 0x6?: Laser firing
  case EclOp::LASER:
    return 1;
  case EclOp::LCMD:
    return 2;
  case EclOp::LLA:
    return 5;
  case EclOp::LLR:
    return 5;
  case EclOp::LL2:
    return 5;
  case EclOp::LDEGA:
    return 3;
  case EclOp::LDEGR:
    return 3;
  case EclOp::LNUMA:
    return 2;
  case EclOp::LNUMR:
    return 2;
  case EclOp::LSPDA:
    return 5;
  case EclOp::LSPDR:
    return 5;
  case EclOp::LCOL:
    return 2;
  case EclOp::LTYPE:
    return 2;
  case EclOp::LWA:
    return 5;
  case EclOp::LDEGS:
    return 1;
  case EclOp::LDEGE:
    return 1;
  case EclOp::LXY:
    return 5;
  case EclOp::LASER2:
    return 1;
  // 0x8?: Large laser & homing
  case EclOp::LLSET:
    return 1;
  case EclOp::LLOPEN:
    return 2;
  case EclOp::LLCLOSE:
    return 2;
  case EclOp::LLCLOSEL:
    return 2;
  case EclOp::LLDEGR:
    return 3;
  case EclOp::HLASER:
    return 1;
  // 0x9?: Flag toggles
  case EclOp::DRAW_ON:
  case EclOp::DRAW_OFF:
  case EclOp::CLIP_ON:
  case EclOp::CLIP_OFF:
  case EclOp::DAMAGE_ON:
  case EclOp::DAMAGE_OFF:
  case EclOp::HITSB_ON:
  case EclOp::HITSB_OFF:
  case EclOp::RLCHG_ON:
  case EclOp::RLCHG_OFF:
    return 1;
  // 0xA?: Special commands
  case EclOp::ANM:
    return 3;
  case EclOp::PSE:
    return 2;
  case EclOp::INT:
    return 2;
  case EclOp::EXDEGD:
    return 2;
  case EclOp::ENEMYSET:
    return 6;
  case EclOp::ENEMYSETD:
    return 7;
  case EclOp::HITXY:
    return 5;
  case EclOp::ITEM:
    return 2;
  case EclOp::STG4EFC:
    return 2;
  case EclOp::ANMEX:
    return 2;
  case EclOp::BITLASER:
    return 2;
  case EclOp::BITATTACK:
    return 5;
  case EclOp::BITCMD:
    return 6;
  case EclOp::BOSSSET:
    return 2;
  case EclOp::CEFC:
    return 6;
  case EclOp::STG3EFC:
    return 1;
  // 0xB?: Register operations
  case EclOp::MOVR:
    return 3;
  case EclOp::MOVC:
    return 6;
  case EclOp::ADD:
    return 3;
  case EclOp::SUB:
    return 3;
  case EclOp::SINL:
    return 3;
  case EclOp::COSL:
    return 3;
  case EclOp::MOD:
    return 6;
  case EclOp::RND:
    return 2;
  case EclOp::CMPR:
    return 3;
  case EclOp::CMPC:
    return 6;
  case EclOp::JL:
    return 5;
  case EclOp::JS:
    return 5;
  case EclOp::INC:
    return 2;
  case EclOp::DEC:
    return 2;
  case EclOp::JEQ:
    return 5;
  default:
    return 1;
  }
}

// ===================================================================
// Decode — tag-dispatched bytecode → typed command struct
// ===================================================================

// Helper for no-parameter commands (length == 1)
template <EclOp Op>
  requires(EclCmdLength(Op) == 1)
inline auto Decode(EclOpTag<Op>, const uint8_t *) {
  return CmdEnd{}; // all 1-byte commands decode to empty struct
}

// --- 0x0?: Control flow ---

inline CmdSetup Decode(EclOpTag<EclOp::SETUP>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U32LEAt(&raw[5])};
}

inline CmdJmp Decode(EclOpTag<EclOp::JMP>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline CmdLoop Decode(EclOpTag<EclOp::LOOP>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U16LEAt(&raw[5])};
}

inline CmdCall Decode(EclOpTag<EclOp::CALL>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline CmdJhpl Decode(EclOpTag<EclOp::JHPL>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U32LEAt(&raw[5])};
}

inline CmdJhps Decode(EclOpTag<EclOp::JHPS>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U32LEAt(&raw[5])};
}

inline CmdJdif Decode(EclOpTag<EclOp::JDIF>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U32LEAt(&raw[5]), U32LEAt(&raw[9]),
          U32LEAt(&raw[13])};
}

inline CmdJdsb Decode(EclOpTag<EclOp::JDSB>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline CmdJfcl Decode(EclOpTag<EclOp::JFCL>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U32LEAt(&raw[5])};
}

inline CmdJfcs Decode(EclOpTag<EclOp::JFCS>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), U32LEAt(&raw[5])};
}

inline CmdSti Decode(EclOpTag<EclOp::STI>, const uint8_t *raw) {
  return {U32LEAt(&raw[1]), static_cast<EclIntVec>(raw[5]), U32LEAt(&raw[6])};
}

inline CmdCli Decode(EclOpTag<EclOp::CLI>, const uint8_t *raw) {
  return {static_cast<EclIntVec>(raw[1])};
}

// --- 0x1?: Movement ---

inline CmdNop Decode(EclOpTag<EclOp::NOP>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

inline CmdNopsc Decode(EclOpTag<EclOp::NOPSC>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

inline CmdMov Decode(EclOpTag<EclOp::MOV>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

inline CmdRol Decode(EclOpTag<EclOp::ROL>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), U16LEAt(&raw[2])};
}

inline CmdLrol Decode(EclOpTag<EclOp::LROL>, const uint8_t *raw) {
  return {I32LEAt(&raw[1]), I32LEAt(&raw[5]), static_cast<int8_t>(raw[9]),
          U16LEAt(&raw[10])};
}

inline CmdWavx Decode(EclOpTag<EclOp::WAVX>, const uint8_t *raw) {
  return {I32LEAt(&raw[1]), raw[5], static_cast<int8_t>(raw[6]),
          U16LEAt(&raw[7])};
}

inline CmdWavy Decode(EclOpTag<EclOp::WAVY>, const uint8_t *raw) {
  return {I32LEAt(&raw[1]), raw[5], static_cast<int8_t>(raw[6]),
          U16LEAt(&raw[7])};
}

inline CmdMxa Decode(EclOpTag<EclOp::MXA>, const uint8_t *raw) {
  return {U16LEAt(&raw[1]), U16LEAt(&raw[3])};
}

inline CmdMya Decode(EclOpTag<EclOp::MYA>, const uint8_t *raw) {
  return {U16LEAt(&raw[1]), U16LEAt(&raw[3])};
}

inline CmdMxya Decode(EclOpTag<EclOp::MXYA>, const uint8_t *raw) {
  return {U16LEAt(&raw[1]), U16LEAt(&raw[3]), U16LEAt(&raw[5])};
}

inline CmdMxs Decode(EclOpTag<EclOp::MXS>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

inline CmdMys Decode(EclOpTag<EclOp::MYS>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

inline CmdMxys Decode(EclOpTag<EclOp::MXYS>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

inline CmdAcc Decode(EclOpTag<EclOp::ACC>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), U16LEAt(&raw[2])};
}

inline CmdGrax Decode(EclOpTag<EclOp::GRAX>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1])};
}

// --- 0x2?: Value assignment ---

inline CmdDega Decode(EclOpTag<EclOp::DEGA>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdDegr Decode(EclOpTag<EclOp::DEGR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1])};
}

inline CmdSpda Decode(EclOpTag<EclOp::SPDA>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdSpdr Decode(EclOpTag<EclOp::SPDR>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdXya Decode(EclOpTag<EclOp::XYA>, const uint8_t *raw) {
  return {U16LEAt(&raw[1]), U16LEAt(&raw[3])};
}

inline CmdXyr Decode(EclOpTag<EclOp::XYR>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3]))};
}

inline CmdXyl Decode(EclOpTag<EclOp::XYL>, const uint8_t *raw) {
  return {U16LEAt(&raw[1])};
}

// --- 0x4?: Bullet firing ---

inline CmdT2item Decode(EclOpTag<EclOp::T2ITEM>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdTauto Decode(EclOpTag<EclOp::TAUTO>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdTxyr Decode(EclOpTag<EclOp::TXYR>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3]))};
}

inline CmdTcmd Decode(EclOpTag<EclOp::TCMD>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdTdega Decode(EclOpTag<EclOp::TDEGA>, const uint8_t *raw) {
  return {raw[1], raw[2]};
}

inline CmdTdegr Decode(EclOpTag<EclOp::TDEGR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), static_cast<int8_t>(raw[2])};
}

inline CmdTnuma Decode(EclOpTag<EclOp::TNUMA>, const uint8_t *raw) {
  return {raw[1], raw[2]};
}

inline CmdTnumr Decode(EclOpTag<EclOp::TNUMR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), static_cast<int8_t>(raw[2])};
}

inline CmdTspda Decode(EclOpTag<EclOp::TSPDA>, const uint8_t *raw) {
  return {raw[1], static_cast<int8_t>(raw[2])};
}

inline CmdTspdr Decode(EclOpTag<EclOp::TSPDR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), static_cast<int8_t>(raw[2])};
}

inline CmdTopt Decode(EclOpTag<EclOp::TOPT>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdTtype Decode(EclOpTag<EclOp::TTYPE>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdTcol Decode(EclOpTag<EclOp::TCOL>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdTvdeg Decode(EclOpTag<EclOp::TVDEG>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1])};
}

inline CmdTrep Decode(EclOpTag<EclOp::TREP>, const uint8_t *raw) {
  return {raw[1]};
}

// --- 0x6?: Laser firing ---

inline CmdLcmd Decode(EclOpTag<EclOp::LCMD>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLla Decode(EclOpTag<EclOp::LLA>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdLlr Decode(EclOpTag<EclOp::LLR>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdLl2 Decode(EclOpTag<EclOp::LL2>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdLdega Decode(EclOpTag<EclOp::LDEGA>, const uint8_t *raw) {
  return {raw[1], raw[2]};
}

inline CmdLdegr Decode(EclOpTag<EclOp::LDEGR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), static_cast<int8_t>(raw[2])};
}

inline CmdLnumpa Decode(EclOpTag<EclOp::LNUMA>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLnumr Decode(EclOpTag<EclOp::LNUMR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1])};
}

inline CmdLspda Decode(EclOpTag<EclOp::LSPDA>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdLspdr Decode(EclOpTag<EclOp::LSPDR>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdLcol Decode(EclOpTag<EclOp::LCOL>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLtype Decode(EclOpTag<EclOp::LTYPE>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLwa Decode(EclOpTag<EclOp::LWA>, const uint8_t *raw) {
  return {I32LEAt(&raw[1])};
}

inline CmdLxy Decode(EclOpTag<EclOp::LXY>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3]))};
}

// --- 0x8?: Large laser & homing ---

inline CmdLlopen Decode(EclOpTag<EclOp::LLOPEN>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLlclose Decode(EclOpTag<EclOp::LLCLOSE>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLlclosel Decode(EclOpTag<EclOp::LLCLOSEL>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdLldegr Decode(EclOpTag<EclOp::LLDEGR>, const uint8_t *raw) {
  return {static_cast<int8_t>(raw[1]), raw[2]};
}

// --- 0xA?: Special commands ---

inline CmdAnm Decode(EclOpTag<EclOp::ANM>, const uint8_t *raw) {
  return {raw[1], static_cast<int8_t>(raw[2])};
}

inline CmdAnmex Decode(EclOpTag<EclOp::ANMEX>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdPse Decode(EclOpTag<EclOp::PSE>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdInt Decode(EclOpTag<EclOp::INT>, const uint8_t *raw) {
  return {static_cast<EclIntType>(raw[1])};
}

inline CmdExdegd Decode(EclOpTag<EclOp::EXDEGD>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdEnemyset Decode(EclOpTag<EclOp::ENEMYSET>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), raw[5]};
}

inline CmdEnemysetd Decode(EclOpTag<EclOp::ENEMYSETD>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), static_cast<EclReg>(raw[5]),
          raw[6]};
}

inline CmdHitxy Decode(EclOpTag<EclOp::HITXY>, const uint8_t *raw) {
  return {U16LEAt(&raw[1]), U16LEAt(&raw[3])};
}

inline CmdItem Decode(EclOpTag<EclOp::ITEM>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdStg4efc Decode(EclOpTag<EclOp::STG4EFC>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdBitlaser Decode(EclOpTag<EclOp::BITLASER>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdBitattack Decode(EclOpTag<EclOp::BITATTACK>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline CmdBitcmd Decode(EclOpTag<EclOp::BITCMD>, const uint8_t *raw) {
  return {raw[1], I32LEAt(&raw[2])};
}

inline CmdBossset Decode(EclOpTag<EclOp::BOSSSET>, const uint8_t *raw) {
  return {raw[1]};
}

inline CmdCefc Decode(EclOpTag<EclOp::CEFC>, const uint8_t *raw) {
  return {static_cast<int16_t>(U16LEAt(&raw[1])),
          static_cast<int16_t>(U16LEAt(&raw[3])), raw[5]};
}

// --- 0xB?: Register operations ---

inline CmdMovr Decode(EclOpTag<EclOp::MOVR>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), static_cast<EclReg>(raw[2])};
}

inline CmdMovc Decode(EclOpTag<EclOp::MOVC>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), U32LEAt(&raw[2])};
}

inline CmdAdd Decode(EclOpTag<EclOp::ADD>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), static_cast<EclReg>(raw[2])};
}

inline CmdSub Decode(EclOpTag<EclOp::SUB>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), static_cast<EclReg>(raw[2])};
}

inline CmdSinl Decode(EclOpTag<EclOp::SINL>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), static_cast<EclReg>(raw[2])};
}

inline CmdCosl Decode(EclOpTag<EclOp::COSL>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), static_cast<EclReg>(raw[2])};
}

inline CmdMod Decode(EclOpTag<EclOp::MOD>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), U32LEAt(&raw[2])};
}

inline CmdRnd Decode(EclOpTag<EclOp::RND>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1])};
}

inline CmdCmpr Decode(EclOpTag<EclOp::CMPR>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), static_cast<EclReg>(raw[2])};
}

inline CmdCmpc Decode(EclOpTag<EclOp::CMPC>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1]), U32LEAt(&raw[2])};
}

inline CmdInc Decode(EclOpTag<EclOp::INC>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1])};
}

inline CmdDec Decode(EclOpTag<EclOp::DEC>, const uint8_t *raw) {
  return {static_cast<EclReg>(raw[1])};
}

inline CmdJl Decode(EclOpTag<EclOp::JL>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline CmdJs Decode(EclOpTag<EclOp::JS>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}

inline CmdJeq Decode(EclOpTag<EclOp::JEQ>, const uint8_t *raw) {
  return {U32LEAt(&raw[1])};
}
