///
/// script_tool - ECL/SCL disassembler and assembler
///
/// Usage:
///   script_tool disasm-scl <in_binary> <out_text>
///   script_tool asm-scl   <in_text> <out_binary>
///   script_tool asm-text <in_text> <out_binary>
///   script_tool disasm-ecl <in_binary> <out_text>
///   script_tool asm-ecl   <in_text> <out_binary>
///

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "util/byte_io.h"
#include "util/endian.h"
#include "util/text_id.h"

// ============================================================================
// ECL command length table (from ECL_LEN.h)
// ============================================================================

static const uint8_t ecl_cmd_len[256] = {
    9, 1, 5, 7, 5, 1, 9, 9, 17, 5, 9, 9, 10, 2, 0, 0, 3, 3, 3, 4, 12, 9, 9, 5,
    5, 7, 3, 3, 3, 4, 7, 2, 2,  2, 1, 1, 5,  5, 5, 5, 1, 1, 1, 1, 1,  1, 3, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1, 2, 5, 2, 3,  3, 3, 3,
    3, 3, 2, 2, 2, 2, 2, 1, 1,  1, 1, 1, 2,  1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,
    1, 2, 5, 5, 5, 3, 3, 2, 2,  5, 5, 2, 2,  5, 1, 1, 5, 1, 0, 0, 0,  0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1,  2, 2, 2, 3,  1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 3, 2, 2, 2, 6,  7, 5, 2,
    2, 2, 2, 5, 6, 2, 6, 1, 3,  6, 3, 3, 3,  3, 6, 2, 3, 6, 5, 5, 2,  2, 5, 0,
};

// ============================================================================
// Operand type descriptors
// ============================================================================

enum class ArgType {
  U8,
  I8,
  U16,
  I16,
  U32,
  I32,
  Label,  // @label → u32LE
  Vector, // STI/CLI vector (BOSSLEFT/HP/TIMER/BITLEFT) → u8
  String, // NUL-terminated string (SCL MSG only)
};

struct ArgDesc {
  const char *name;
  ArgType type;
};

struct EclOpInfo {
  const char *name;
  uint8_t length;
  ArgDesc args[4];
  int arg_count;
};

struct SclOpInfo {
  const char *name;
  ArgDesc args[4];
  int arg_count;
};

// ============================================================================
// ECL opcode info table
// ============================================================================

static const EclOpInfo *ecl_op_info(uint8_t op) {
  // clang-format off
    switch (op) {
    case 0x00: { static const EclOpInfo i = {"SETUP",      9,  {{"hp", ArgType::U32}, {"score", ArgType::U32}}, 2}; return &i; }
    case 0x01: { static const EclOpInfo i = {"END",        1,  {}, 0}; return &i; }
    case 0x02: { static const EclOpInfo i = {"JMP",        5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    case 0x03: { static const EclOpInfo i = {"LOOP",       7,  {{"jmp", ArgType::Label}, {"count", ArgType::U16}}, 2}; return &i; }
    case 0x04: { static const EclOpInfo i = {"CALL",       5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    case 0x05: { static const EclOpInfo i = {"RET",        1,  {}, 0}; return &i; }
    case 0x06: { static const EclOpInfo i = {"JHPL",       9,  {{"hp", ArgType::U32}, {"jmp", ArgType::Label}}, 2}; return &i; }
    case 0x07: { static const EclOpInfo i = {"JHPS",       9,  {{"hp", ArgType::U32}, {"jmp", ArgType::Label}}, 2}; return &i; }
    case 0x08: { static const EclOpInfo i = {"JDIF",      17,  {{"easy", ArgType::Label}, {"norm", ArgType::Label}, {"hard", ArgType::Label}, {"luna", ArgType::Label}}, 4}; return &i; }
    case 0x09: { static const EclOpInfo i = {"JDSB",       5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    case 0x0A: { static const EclOpInfo i = {"JFCL",       9,  {{"frame", ArgType::U32}, {"jmp", ArgType::Label}}, 2}; return &i; }
    case 0x0B: { static const EclOpInfo i = {"JFCS",       9,  {{"frame", ArgType::U32}, {"jmp", ArgType::Label}}, 2}; return &i; }
    case 0x0C: { static const EclOpInfo i = {"STI",       10,  {{"jmp", ArgType::Label}, {"vector", ArgType::Vector}, {"val", ArgType::U32}}, 3}; return &i; }
    case 0x0D: { static const EclOpInfo i = {"CLI",        2,  {{"vector", ArgType::Vector}}, 1}; return &i; }
    case 0x10: { static const EclOpInfo i = {"NOP",        3,  {{"count", ArgType::U16}}, 1}; return &i; }
    case 0x11: { static const EclOpInfo i = {"NOPSC",      3,  {{"count", ArgType::U16}}, 1}; return &i; }
    case 0x12: { static const EclOpInfo i = {"MOV",        3,  {{"count", ArgType::U16}}, 1}; return &i; }
    case 0x13: { static const EclOpInfo i = {"ROL",        4,  {{"deg", ArgType::I8}, {"count", ArgType::U16}}, 2}; return &i; }
    case 0x14: { static const EclOpInfo i = {"LROL",      12,  {{"vx", ArgType::I32}, {"vy", ArgType::I32}, {"deg", ArgType::I8}, {"count", ArgType::U16}}, 4}; return &i; }
    case 0x15: { static const EclOpInfo i = {"WAVX",       9,  {{"vx", ArgType::I32}, {"amp", ArgType::U8}, {"vd", ArgType::I8}, {"count", ArgType::U16}}, 4}; return &i; }
    case 0x16: { static const EclOpInfo i = {"WAVY",       9,  {{"vy", ArgType::I32}, {"amp", ArgType::U8}, {"vd", ArgType::I8}, {"count", ArgType::U16}}, 4}; return &i; }
    case 0x17: { static const EclOpInfo i = {"MXA",        5,  {{"x", ArgType::U16}, {"count", ArgType::U16}}, 2}; return &i; }
    case 0x18: { static const EclOpInfo i = {"MYA",        5,  {{"y", ArgType::U16}, {"count", ArgType::U16}}, 2}; return &i; }
    case 0x19: { static const EclOpInfo i = {"MXYA",       7,  {{"x", ArgType::U16}, {"y", ArgType::U16}, {"count", ArgType::U16}}, 3}; return &i; }
    case 0x1A: { static const EclOpInfo i = {"MXS",        3,  {{"count", ArgType::U16}}, 1}; return &i; }
    case 0x1B: { static const EclOpInfo i = {"MYS",        3,  {{"count", ArgType::U16}}, 1}; return &i; }
    case 0x1C: { static const EclOpInfo i = {"MXYS",       3,  {{"count", ArgType::U16}}, 1}; return &i; }
    case 0x1D: { static const EclOpInfo i = {"ACC",        4,  {{"accel", ArgType::I8}, {"count", ArgType::U16}}, 2}; return &i; }
    case 0x1E: { static const EclOpInfo i = {"ACCXYA",     7,  {{"dx", ArgType::I16}, {"dy", ArgType::I16}, {"v", ArgType::I16}}, 3}; return &i; }
    case 0x1F: { static const EclOpInfo i = {"GRAX",       2,  {{"gravity", ArgType::I8}}, 1}; return &i; }
    case 0x20: { static const EclOpInfo i = {"DEGA",       2,  {{"angle", ArgType::U8}}, 1}; return &i; }
    case 0x21: { static const EclOpInfo i = {"DEGR",       2,  {{"angle", ArgType::I8}}, 1}; return &i; }
    case 0x22: { static const EclOpInfo i = {"DEGX",       1,  {}, 0}; return &i; }
    case 0x23: { static const EclOpInfo i = {"DEGS",       1,  {}, 0}; return &i; }
    case 0x24: { static const EclOpInfo i = {"SPDA",       5,  {{"speed", ArgType::I32}}, 1}; return &i; }
    case 0x25: { static const EclOpInfo i = {"SPDR",       5,  {{"speed", ArgType::I32}}, 1}; return &i; }
    case 0x26: { static const EclOpInfo i = {"XYA",        5,  {{"x", ArgType::I16}, {"y", ArgType::I16}}, 2}; return &i; }
    case 0x27: { static const EclOpInfo i = {"XYR",        5,  {{"dx", ArgType::I16}, {"dy", ArgType::I16}}, 2}; return &i; }
    case 0x28: { static const EclOpInfo i = {"DEGXU",      1,  {}, 0}; return &i; }
    case 0x29: { static const EclOpInfo i = {"DEGXD",      1,  {}, 0}; return &i; }
    case 0x2A: { static const EclOpInfo i = {"DEGEX",      1,  {}, 0}; return &i; }
    case 0x2B: { static const EclOpInfo i = {"XYS",        1,  {}, 0}; return &i; }
    case 0x2C: { static const EclOpInfo i = {"DEGX2",      1,  {}, 0}; return &i; }
    case 0x2D: { static const EclOpInfo i = {"XYRND",      1,  {}, 0}; return &i; }
    case 0x2E: { static const EclOpInfo i = {"XYL",        3,  {{"len", ArgType::I16}}, 1}; return &i; }
    case 0x40: { static const EclOpInfo i = {"TAMA",       1,  {}, 0}; return &i; }
    case 0x41: { static const EclOpInfo i = {"TAUTO",      2,  {{"interval", ArgType::U8}}, 1}; return &i; }
    case 0x42: { static const EclOpInfo i = {"TXYR",       5,  {{"dx", ArgType::I16}, {"dy", ArgType::I16}}, 2}; return &i; }
    case 0x43: { static const EclOpInfo i = {"TCMD",       2,  {{"cmd", ArgType::U8}}, 1}; return &i; }
    case 0x44: { static const EclOpInfo i = {"TDEGA",      3,  {{"angle", ArgType::U8}, {"dw", ArgType::U8}}, 2}; return &i; }
    case 0x45: { static const EclOpInfo i = {"TDEGR",      3,  {{"angle", ArgType::I8}, {"dw", ArgType::I8}}, 2}; return &i; }
    case 0x46: { static const EclOpInfo i = {"TNUMA",      3,  {{"n", ArgType::U8}, {"ns", ArgType::U8}}, 2}; return &i; }
    case 0x47: { static const EclOpInfo i = {"TNUMR",      3,  {{"n", ArgType::I8}, {"ns", ArgType::I8}}, 2}; return &i; }
    case 0x48: { static const EclOpInfo i = {"TSPDA",      3,  {{"v", ArgType::U8}, {"a", ArgType::I8}}, 2}; return &i; }
    case 0x49: { static const EclOpInfo i = {"TSPDR",      3,  {{"v", ArgType::I8}, {"a", ArgType::I8}}, 2}; return &i; }
    case 0x4A: { static const EclOpInfo i = {"TOPT",       2,  {{"opt", ArgType::U8}}, 1}; return &i; }
    case 0x4B: { static const EclOpInfo i = {"TTYPE",      2,  {{"type", ArgType::U8}}, 1}; return &i; }
    case 0x4C: { static const EclOpInfo i = {"TCOL",       2,  {{"color", ArgType::U8}}, 1}; return &i; }
    case 0x4D: { static const EclOpInfo i = {"TVDEG",      2,  {{"vd", ArgType::I8}}, 1}; return &i; }
    case 0x4E: { static const EclOpInfo i = {"TREP",       2,  {{"rep", ArgType::U8}}, 1}; return &i; }
    case 0x4F: { static const EclOpInfo i = {"TDEGS",      1,  {}, 0}; return &i; }
    case 0x50: { static const EclOpInfo i = {"TDEGE",      1,  {}, 0}; return &i; }
    case 0x51: { static const EclOpInfo i = {"TAMA2",      1,  {}, 0}; return &i; }
    case 0x52: { static const EclOpInfo i = {"TCLR",       1,  {}, 0}; return &i; }
    case 0x53: { static const EclOpInfo i = {"TAMAL",      1,  {}, 0}; return &i; }
    case 0x54: { static const EclOpInfo i = {"T2ITEM",     2,  {{"pct", ArgType::U8}}, 1}; return &i; }
    case 0x55: { static const EclOpInfo i = {"TAMAEX",     1,  {}, 0}; return &i; }
    case 0x60: { static const EclOpInfo i = {"LASER",      1,  {}, 0}; return &i; }
    case 0x61: { static const EclOpInfo i = {"LCMD",       2,  {{"cmd", ArgType::U8}}, 1}; return &i; }
    case 0x62: { static const EclOpInfo i = {"LLA",        5,  {{"len", ArgType::I32}}, 1}; return &i; }
    case 0x63: { static const EclOpInfo i = {"LLR",        5,  {{"len", ArgType::I32}}, 1}; return &i; }
    case 0x64: { static const EclOpInfo i = {"LL2",        5,  {{"l2", ArgType::I32}}, 1}; return &i; }
    case 0x65: { static const EclOpInfo i = {"LDEGA",      3,  {{"angle", ArgType::U8}, {"dw", ArgType::U8}}, 2}; return &i; }
    case 0x66: { static const EclOpInfo i = {"LDEGR",      3,  {{"angle", ArgType::I8}, {"dw", ArgType::I8}}, 2}; return &i; }
    case 0x67: { static const EclOpInfo i = {"LNUMA",      2,  {{"n", ArgType::U8}}, 1}; return &i; }
    case 0x68: { static const EclOpInfo i = {"LNUMR",      2,  {{"n", ArgType::I8}}, 1}; return &i; }
    case 0x69: { static const EclOpInfo i = {"LSPDA",      5,  {{"v", ArgType::I32}}, 1}; return &i; }
    case 0x6A: { static const EclOpInfo i = {"LSPDR",      5,  {{"v", ArgType::I32}}, 1}; return &i; }
    case 0x6B: { static const EclOpInfo i = {"LCOL",       2,  {{"color", ArgType::U8}}, 1}; return &i; }
    case 0x6C: { static const EclOpInfo i = {"LTYPE",      2,  {{"type", ArgType::U8}}, 1}; return &i; }
    case 0x6D: { static const EclOpInfo i = {"LWA",        5,  {{"w", ArgType::I32}}, 1}; return &i; }
    case 0x6E: { static const EclOpInfo i = {"LDEGS",      1,  {}, 0}; return &i; }
    case 0x6F: { static const EclOpInfo i = {"LDEGE",      1,  {}, 0}; return &i; }
    case 0x70: { static const EclOpInfo i = {"LXY",        5,  {{"x", ArgType::I16}, {"y", ArgType::I16}}, 2}; return &i; }
    case 0x71: { static const EclOpInfo i = {"LASER2",     1,  {}, 0}; return &i; }
    case 0x80: { static const EclOpInfo i = {"LLSET",      1,  {}, 0}; return &i; }
    case 0x81: { static const EclOpInfo i = {"LLOPEN",     2,  {{"id", ArgType::U8}}, 1}; return &i; }
    case 0x82: { static const EclOpInfo i = {"LLCLOSE",    2,  {{"id", ArgType::U8}}, 1}; return &i; }
    case 0x83: { static const EclOpInfo i = {"LLCLOSEL",   2,  {{"id", ArgType::U8}}, 1}; return &i; }
    case 0x84: { static const EclOpInfo i = {"LLDEGR",     3,  {{"id", ArgType::U8}, {"deg", ArgType::I8}}, 2}; return &i; }
    case 0x85: { static const EclOpInfo i = {"HLASER",     1,  {}, 0}; return &i; }
    case 0x90: { static const EclOpInfo i = {"DRAW_ON",    1,  {}, 0}; return &i; }
    case 0x91: { static const EclOpInfo i = {"DRAW_OFF",   1,  {}, 0}; return &i; }
    case 0x92: { static const EclOpInfo i = {"CLIP_ON",    1,  {}, 0}; return &i; }
    case 0x93: { static const EclOpInfo i = {"CLIP_OFF",   1,  {}, 0}; return &i; }
    case 0x94: { static const EclOpInfo i = {"DAMAGE_ON",  1,  {}, 0}; return &i; }
    case 0x95: { static const EclOpInfo i = {"DAMAGE_OFF", 1,  {}, 0}; return &i; }
    case 0x96: { static const EclOpInfo i = {"HITSB_ON",   1,  {}, 0}; return &i; }
    case 0x97: { static const EclOpInfo i = {"HITSB_OFF",  1,  {}, 0}; return &i; }
    case 0x98: { static const EclOpInfo i = {"RLCHG_ON",   1,  {}, 0}; return &i; }
    case 0x99: { static const EclOpInfo i = {"RLCHG_OFF",  1,  {}, 0}; return &i; }
    case 0xA0: { static const EclOpInfo i = {"ANM",        3,  {{"pattern", ArgType::U8}, {"speed", ArgType::I8}}, 2}; return &i; }
    case 0xA1: { static const EclOpInfo i = {"PSE",        2,  {{"id", ArgType::U8}}, 1}; return &i; }
    case 0xA2: { static const EclOpInfo i = {"INT",        2,  {{"id", ArgType::U8}}, 1}; return &i; }
    case 0xA3: { static const EclOpInfo i = {"EXDEGD",     2,  {{"deg", ArgType::U8}}, 1}; return &i; }
    case 0xA4: { static const EclOpInfo i = {"ENEMYSET",   6,  {{"dx", ArgType::I16}, {"dy", ArgType::I16}, {"id", ArgType::U8}}, 3}; return &i; }
    case 0xA5: { static const EclOpInfo i = {"ENEMYSETD",  7,  {{"dx", ArgType::I16}, {"dy", ArgType::I16}, {"reg", ArgType::U8}, {"id", ArgType::U8}}, 4}; return &i; }
    case 0xA6: { static const EclOpInfo i = {"HITXY",      5,  {{"w", ArgType::U16}, {"h", ArgType::U16}}, 2}; return &i; }
    case 0xA7: { static const EclOpInfo i = {"ITEM",       2,  {{"type", ArgType::U8}}, 1}; return &i; }
    case 0xA8: { static const EclOpInfo i = {"STG4EFC",    2,  {{"cmd", ArgType::U8}}, 1}; return &i; }
    case 0xA9: { static const EclOpInfo i = {"ANMEX",      2,  {{"pattern", ArgType::U8}}, 1}; return &i; }
    case 0xAA: { static const EclOpInfo i = {"BITLASER",   2,  {{"cmd", ArgType::U8}}, 1}; return &i; }
    case 0xAB: { static const EclOpInfo i = {"BITATTACK",  5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    case 0xAC: { static const EclOpInfo i = {"BITCMD",     6,  {{"cmd", ArgType::U8}, {"val", ArgType::I32}}, 2}; return &i; }
    case 0xAD: { static const EclOpInfo i = {"BOSSSET",    2,  {{"id", ArgType::U8}}, 1}; return &i; }
    case 0xAE: { static const EclOpInfo i = {"CEFC",       6,  {{"x", ArgType::I16}, {"y", ArgType::I16}, {"type", ArgType::U8}}, 3}; return &i; }
    case 0xAF: { static const EclOpInfo i = {"STG3EFC",    1,  {}, 0}; return &i; }
    case 0xB0: { static const EclOpInfo i = {"MOVR",       3,  {{"dst", ArgType::U8}, {"src", ArgType::U8}}, 2}; return &i; }
    case 0xB1: { static const EclOpInfo i = {"MOVC",       6,  {{"dst", ArgType::U8}, {"val", ArgType::U32}}, 2}; return &i; }
    case 0xB2: { static const EclOpInfo i = {"ADD",        3,  {{"dst", ArgType::U8}, {"src", ArgType::U8}}, 2}; return &i; }
    case 0xB3: { static const EclOpInfo i = {"SUB",        3,  {{"dst", ArgType::U8}, {"src", ArgType::U8}}, 2}; return &i; }
    case 0xB4: { static const EclOpInfo i = {"SINL",       3,  {{"len", ArgType::U8}, {"deg", ArgType::U8}}, 2}; return &i; }
    case 0xB5: { static const EclOpInfo i = {"COSL",       3,  {{"len", ArgType::U8}, {"deg", ArgType::U8}}, 2}; return &i; }
    case 0xB6: { static const EclOpInfo i = {"MOD",        6,  {{"reg", ArgType::U8}, {"div", ArgType::U32}}, 2}; return &i; }
    case 0xB7: { static const EclOpInfo i = {"RND",        2,  {{"reg", ArgType::U8}}, 1}; return &i; }
    case 0xB8: { static const EclOpInfo i = {"CMPR",       3,  {{"reg0", ArgType::U8}, {"reg1", ArgType::U8}}, 2}; return &i; }
    case 0xB9: { static const EclOpInfo i = {"CMPC",       6,  {{"reg", ArgType::U8}, {"val", ArgType::U32}}, 2}; return &i; }
    case 0xBA: { static const EclOpInfo i = {"JL",         5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    case 0xBB: { static const EclOpInfo i = {"JS",         5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    case 0xBC: { static const EclOpInfo i = {"INC",        2,  {{"reg", ArgType::U8}}, 1}; return &i; }
    case 0xBD: { static const EclOpInfo i = {"DEC",        2,  {{"reg", ArgType::U8}}, 1}; return &i; }
    case 0xBE: { static const EclOpInfo i = {"JEQ",        5,  {{"jmp", ArgType::Label}}, 1}; return &i; }
    default: return nullptr;
    }
  // clang-format on
}

// ============================================================================
// SCL opcode info table
// ============================================================================

static const SclOpInfo *scl_op_info(uint8_t op) {
  static const SclOpInfo table[] = {
      {"TIME", {{"frame", ArgType::U32}}, 1},
      {"ENEMY",
       {{"x", ArgType::I16}, {"y", ArgType::I16}, {"id", ArgType::U8}},
       3},
      {"SSP", {{"speed", ArgType::I16}}, 1},
      {"EFC", {{"type", ArgType::U8}}, 1},
      {"END", {}, 0},
      {"BOSS",
       {{"x", ArgType::I16}, {"y", ArgType::I16}, {"id", ArgType::U8}},
       3},
      {"MWOPEN", {}, 0},
      {"MWCLOSE", {}, 0},
      {"MSG", {{"text", ArgType::String}}, 1},
      {"KEY", {}, 0},
      {"NPG", {}, 0},
      {"FACE", {{"id", ArgType::U8}}, 1},
      {"MUSIC", {{"id", ArgType::U8}}, 1},
      {"BOSSDEAD", {}, 0},
      {"LOADFACE", {{"surf", ArgType::U8}, {"file", ArgType::U8}}, 2},
      {"WAITEX", {{"cond", ArgType::U8}, {"opt", ArgType::U32}}, 2},
      {"STAGECLEAR", {}, 0},
      {"MAPPALETTE", {}, 0},
      {"GAMECLEAR", {}, 0},
      {"DELENEMY", {}, 0},
      {"ENEMYPALETTE", {}, 0},
      {"STAFF", {{"id", ArgType::U8}}, 1},
      {"EXTRACLEAR", {}, 0},
      {"MSGREF", {{"id", ArgType::U32}}, 1},
  };
  if (op <= 0x17 && table[op].name)
    return &table[op];
  return nullptr;
}

// ============================================================================
// Symbolic name helpers
// ============================================================================

static const char *stivect_name(uint8_t v) {
  switch (v) {
  case 0:
    return "BOSSLEFT";
  case 1:
    return "HP";
  case 2:
    return "TIMER";
  case 3:
    return "BITLEFT";
  default:
    return nullptr;
  }
}

static int stivect_value(std::string_view s) {
  if (s == "BOSSLEFT")
    return 0;
  if (s == "HP")
    return 1;
  if (s == "TIMER")
    return 2;
  if (s == "BITLEFT")
    return 3;
  return -1;
}

static const char *efc_name(uint8_t t) {
  switch (t) {
  case 0x00:
    return "WARN";
  case 0x01:
    return "WARNSTOP";
  case 0x02:
    return "MUSICFADE";
  case 0x03:
    return "STG2BOSS";
  case 0x04:
    return "RASTERON";
  case 0x05:
    return "RASTEROFF";
  case 0x06:
    return "CFADEIN";
  case 0x07:
    return "CFADEOUT";
  case 0x08:
    return "STG3BOSS";
  case 0x09:
    return "STG3RESET";
  case 0x0A:
    return "STG6CUBE";
  case 0x0B:
    return "STG6RNDECL";
  case 0x0C:
    return "STG4ROCK";
  case 0x0D:
    return "STG4LEAVE";
  case 0x0E:
    return "WHITEIN";
  case 0x0F:
    return "WHITEOUT";
  case 0x10:
    return "LOADEX01";
  case 0x11:
    return "LOADEX02";
  case 0x12:
    return "STG6RASTER";
  default:
    return nullptr;
  }
}

static int efc_value(std::string_view s) {
  static const std::unordered_map<std::string_view, int> map = {
      {"WARN", 0x00},       {"WARNSTOP", 0x01},  {"MUSICFADE", 0x02},
      {"STG2BOSS", 0x03},   {"RASTERON", 0x04},  {"RASTEROFF", 0x05},
      {"CFADEIN", 0x06},    {"CFADEOUT", 0x07},  {"STG3BOSS", 0x08},
      {"STG3RESET", 0x09},  {"STG6CUBE", 0x0A},  {"STG6RNDECL", 0x0B},
      {"STG4ROCK", 0x0C},   {"STG4LEAVE", 0x0D}, {"WHITEIN", 0x0E},
      {"WHITEOUT", 0x0F},   {"LOADEX01", 0x10},  {"LOADEX02", 0x11},
      {"STG6RASTER", 0x12},
  };
  auto it = map.find(s);
  return (it != map.end()) ? it->second : -1;
}

static const char *scwait_name(uint8_t c) {
  switch (c) {
  case 0:
    return "BOSSLEFT";
  case 1:
    return "BOSSHP";
  default:
    return nullptr;
  }
}

static int scwait_value(std::string_view s) {
  if (s == "BOSSLEFT")
    return 0;
  if (s == "BOSSHP")
    return 1;
  return -1;
}

// ============================================================================
// File I/O
// ============================================================================

static std::vector<uint8_t> read_file(const char *path) {
  auto *fp = std::fopen(path, "rb");
  if (!fp) {
    std::println(stderr, "Error: Cannot open '{}'", path);
    return {};
  }
  std::fseek(fp, 0, SEEK_END);
  const auto size = std::ftell(fp);
  std::fseek(fp, 0, SEEK_SET);
  std::vector<uint8_t> buf(size);
  if (std::fread(buf.data(), 1, size, fp) != static_cast<size_t>(size)) {
    std::println(stderr, "Error: Failed to read '{}'", path);
    std::fclose(fp);
    return {};
  }
  std::fclose(fp);
  return buf;
}

static bool write_file(const char *path, const std::vector<uint8_t> &data) {
  auto *fp = std::fopen(path, "wb");
  if (!fp) {
    std::println(stderr, "Error: Cannot write '{}'", path);
    return false;
  }
  if (std::fwrite(data.data(), 1, data.size(), fp) != data.size()) {
    std::println(stderr, "Error: Failed to write '{}'", path);
    std::fclose(fp);
    return false;
  }
  std::fclose(fp);
  return true;
}

// ============================================================================
// String escaping
// ============================================================================

static std::string escape_string(const uint8_t *data, size_t len) {
  std::string out;
  out.reserve(len + 2);
  for (size_t i = 0; i < len; i++) {
    uint8_t c = data[i];
    if (c >= 0x20 && c <= 0x7E && c != '"' && c != '\\') {
      out += static_cast<char>(c);
    } else if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else {
      out += std::format("\\x{:02X}", c);
    }
  }
  return out;
}

static std::vector<uint8_t> unescape_string(std::string_view s) {
  std::vector<uint8_t> out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      switch (s[i + 1]) {
      case 'x':
      case 'X': {
        if (i + 3 < s.size() &&
            std::isxdigit(static_cast<unsigned char>(s[i + 2])) &&
            std::isxdigit(static_cast<unsigned char>(s[i + 3]))) {
          char hex[3] = {s[i + 2], s[i + 3], 0};
          char *end = nullptr;
          out.push_back(static_cast<uint8_t>(std::strtoul(hex, &end, 16)));
          i += 3;
        } else {
          out.push_back('\\');
        }
        break;
      }
      case '"':
        out.push_back('"');
        i++;
        break;
      case '\\':
        out.push_back('\\');
        i++;
        break;
      case 'n':
        out.push_back('\n');
        i++;
        break;
      case 'r':
        out.push_back('\r');
        i++;
        break;
      case 't':
        out.push_back('\t');
        i++;
        break;
      default:
        out.push_back('\\');
        break;
      }
    } else {
      out.push_back(static_cast<uint8_t>(s[i]));
    }
  }
  return out;
}

static uint32_t text_id_hash(std::string_view key) {
  return util::TextIdFromKey(key);
}

// ============================================================================
// Tokenizer for assembler input
// ============================================================================

enum class TokenKind {
  Label,     // @name: → label definition
  LabelRef,  // @name (in operand position)
  Mnemonic,  // instruction name
  Key,       // key= prefix
  Number,    // decimal or hex integer
  String,    // "quoted string"
  Directive, // .name
  Comment,   // blank / comment
};

struct Token {
  TokenKind kind = TokenKind::Comment;
  std::string text;
  int64_t numval = 0;
};

static std::vector<Token> tokenize_line(std::string_view line, int lineno) {
  std::vector<Token> tokens;
  size_t i = 0;
  while (i < line.size()) {
    char c = line[i];
    if (c == ' ' || c == '\t' || c == '\r') {
      i++;
      continue;
    }
    if (c == ';')
      break;

    if (c == '"') {
      i++;
      std::string str;
      while (i < line.size() && line[i] != '"') {
        if (line[i] == '\\' && i + 1 < line.size()) {
          str += line[i];
          str += line[i + 1];
          i += 2;
        } else {
          str += line[i];
          i++;
        }
      }
      if (i >= line.size()) {
        std::println(stderr, "Line {}: unterminated string", lineno);
        return {};
      }
      i++;
      Token t{TokenKind::String, str, 0};
      tokens.push_back(t);
      continue;
    }

    if (c == '@') {
      size_t start = i;
      i++;
      while (
          i < line.size() &&
          (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_'))
        i++;
      std::string name(line.substr(start + 1, i - start - 1));
      if (name.empty()) {
        i = start + 1;
        continue;
      }
      bool is_def = (i < line.size() && line[i] == ':');
      Token t;
      t.kind = is_def ? TokenKind::Label : TokenKind::LabelRef;
      t.text = name;
      if (is_def)
        i++;
      tokens.push_back(t);
      continue;
    }

    if (c == '.') {
      size_t start = i;
      i++;
      while (i < line.size() &&
             !std::isspace(static_cast<unsigned char>(line[i])) &&
             line[i] != ';')
        i++;
      Token t{TokenKind::Directive,
              std::string(line.substr(start + 1, i - start - 1)), 0};
      tokens.push_back(t);
      continue;
    }

    if (c == '-' || c == '+' || std::isdigit(static_cast<unsigned char>(c))) {
      size_t start = i;
      bool neg = (c == '-');
      if (c == '-' || c == '+')
        i++;
      int base = 10;
      if (i < line.size() && line[i] == '0' && i + 1 < line.size() &&
          (line[i + 1] == 'x' || line[i + 1] == 'X')) {
        i += 2;
        base = 16;
      }
      while (i < line.size() &&
             (base == 16 ? std::isxdigit(static_cast<unsigned char>(line[i]))
                         : std::isdigit(static_cast<unsigned char>(line[i]))))
        i++;
      std::string numstr(line.substr(start, i - start));
      int64_t v = std::strtoll(numstr.c_str(), nullptr, 0);
      Token t{TokenKind::Number, numstr, v};
      tokens.push_back(t);
      continue;
    }

    if (c == '=' && !tokens.empty() &&
        tokens.back().kind == TokenKind::Mnemonic) {
      tokens.back().kind = TokenKind::Key;
      i++;
      continue;
    }

    {
      size_t start = i;
      while (i < line.size() &&
             !std::isspace(static_cast<unsigned char>(line[i])) &&
             line[i] != ';' && line[i] != '=' && line[i] != '"' &&
             line[i] != '@')
        i++;
      std::string word(line.substr(start, i - start));
      if (word.empty())
        continue;
      bool has_eq = (word.back() == '=');
      if (has_eq)
        word.pop_back();

      if (tokens.empty() || tokens.back().kind == TokenKind::Label) {
        Token t{TokenKind::Mnemonic, word, 0};
        tokens.push_back(t);
        if (has_eq)
          tokens.back().kind = TokenKind::Key;
      } else if (tokens.back().kind == TokenKind::Mnemonic ||
                 tokens.back().kind == TokenKind::Key ||
                 tokens.back().kind == TokenKind::Number ||
                 tokens.back().kind == TokenKind::LabelRef ||
                 tokens.back().kind == TokenKind::String) {
        Token t{has_eq ? TokenKind::Key : TokenKind::Mnemonic, word, 0};
        tokens.push_back(t);
      } else {
        Token t{TokenKind::Mnemonic, word, 0};
        tokens.push_back(t);
      }
    }
  }
  if (tokens.empty()) {
    tokens.push_back({TokenKind::Comment, {}, 0});
  }
  return tokens;
}

// ============================================================================
// SCL disassembler
// ============================================================================

static bool cmd_disasm_scl(const char *in_file, const char *out_file) {
  auto data = read_file(in_file);
  if (data.empty())
    return false;

  std::string out;
  size_t pos = 0;
  while (pos < data.size()) {
    uint8_t op = data[pos];
    out += "    ";
    auto *info = scl_op_info(op);

    if (op == 0x08) {
      pos++;
      size_t str_start = pos;
      while (pos < data.size() && data[pos] != 0)
        pos++;
      std::string escaped = escape_string(&data[str_start], pos - str_start);
      out += std::format("MSG \"{}\"\n", escaped);
      if (pos < data.size())
        pos++;
    } else if (info) {
      out += info->name;
      int off = 1;
      for (int ai = 0; ai < info->arg_count; ai++) {
        const auto &arg = info->args[ai];
        out += ' ';
        out += arg.name;
        out += '=';
        switch (arg.type) {
        case ArgType::U8: {
          uint8_t v = data[pos + off];
          if (ai == 0 && op == 0x03) {
            auto *ename = efc_name(v);
            out += ename ? ename : std::to_string(v);
          } else {
            out += std::to_string(v);
          }
          off += 1;
          break;
        }
        case ArgType::I8:
          out += std::to_string(static_cast<int8_t>(data[pos + off]));
          off += 1;
          break;
        case ArgType::I16:
          out += std::to_string(*util::ReadLittleAt<int16_t>(data, pos + off));
          off += 2;
          break;
        case ArgType::U32: {
          uint32_t v = *util::ReadLittleAt<uint32_t>(data, pos + off);
          if (ai == 0 && op == 0x0F) {
            auto *cname = scwait_name(static_cast<uint8_t>(v));
            out += cname ? cname : std::to_string(v);
          } else {
            out += std::to_string(v);
          }
          off += 4;
          break;
        }
        default:
          break;
        }
      }
      out += '\n';
      pos += (op == 0x00)                 ? 5
             : (op == 0x01 || op == 0x05) ? 6
             : (op == 0x02)               ? 3
             : (op == 0x03)               ? 2
             : (op == 0x04 || op == 0x06 || op == 0x07 || op == 0x09 ||
                op == 0x0A || op == 0x0D || op == 0x10 || op == 0x11 ||
                op == 0x12 || op == 0x13 || op == 0x14 || op == 0x16)
                 ? 1
             : (op == 0x0B || op == 0x0C || op == 0x15) ? 2
             : (op == 0x0E)                             ? 3
             : (op == 0x0F)                             ? 6
             : (op == 0x17)                             ? 5
                                                        : 1;
    } else {
      out += std::format("; unknown opcode 0x{:02X} at +0x{:04X}\n", op, pos);
      pos++;
    }
  }

  std::ofstream ofs(out_file);
  if (!ofs) {
    std::println(stderr, "Error: Cannot write '{}'", out_file);
    return false;
  }
  ofs << out;
  std::println("Disassembled SCL to '{}'", out_file);
  return true;
}

// ============================================================================
// SCL assembler
// ============================================================================

static bool cmd_asm_scl(const char *in_file, const char *out_file) {
  std::ifstream ifs(in_file);
  if (!ifs) {
    std::println(stderr, "Error: Cannot open '{}'", in_file);
    return false;
  }

  std::vector<uint8_t> out;
  std::string line;
  int lineno = 0;

  while (std::getline(ifs, line)) {
    lineno++;
    auto tokens = tokenize_line(line, lineno);
    if (tokens.empty() || tokens[0].kind == TokenKind::Comment)
      continue;
    if (tokens[0].kind == TokenKind::Label ||
        tokens[0].kind == TokenKind::Directive)
      continue;

    if (tokens[0].kind != TokenKind::Mnemonic) {
      std::println(stderr, "Line {}: expected mnemonic", lineno);
      return false;
    }

    std::string mnem = tokens[0].text;
    uint8_t op = 0xFF;
    for (int o = 0; o <= 0x17; o++) {
      if (auto *inf = scl_op_info(static_cast<uint8_t>(o))) {
        if (mnem == inf->name) {
          op = static_cast<uint8_t>(o);
          break;
        }
      }
    }
    if (op == 0xFF) {
      std::println(stderr, "Line {}: unknown mnemonic '{}'", lineno, mnem);
      return false;
    }

    auto *info = scl_op_info(op);
    out.push_back(op);

    if (op == 0x08) {
      if (tokens.size() < 2 || tokens[1].kind != TokenKind::String) {
        std::println(stderr, "Line {}: MSG requires a string argument", lineno);
        return false;
      }
      auto str = unescape_string(tokens[1].text);
      out.insert(out.end(), str.begin(), str.end());
      out.push_back(0);
    } else {
      // Build key→value map from tokens
      std::unordered_map<std::string, int64_t> arg_map;
      std::string pending_key;
      for (size_t ti = 1; ti < tokens.size(); ti++) {
        if (tokens[ti].kind == TokenKind::Key) {
          pending_key = tokens[ti].text;
        } else if (tokens[ti].kind == TokenKind::Number ||
                   tokens[ti].kind == TokenKind::LabelRef) {
          std::string key = pending_key.empty()
                                ? info->args[arg_map.size()].name
                                : pending_key;
          pending_key.clear();
          if (tokens[ti].kind == TokenKind::LabelRef && op != 0x17) {
            std::println(stderr,
                         "Line {}: label references not supported in SCL",
                         lineno);
            return false;
          }
          arg_map[key] = tokens[ti].kind == TokenKind::LabelRef
                             ? text_id_hash(tokens[ti].text)
                             : tokens[ti].numval;
        } else if (tokens[ti].kind == TokenKind::Mnemonic) {
          std::string key = pending_key.empty()
                                ? info->args[arg_map.size()].name
                                : pending_key;
          pending_key.clear();
          if (op == 0x03 && key == "type") {
            int ev = efc_value(tokens[ti].text);
            if (ev >= 0)
              arg_map[key] = ev;
          } else if (op == 0x0F && key == "cond") {
            int wv = scwait_value(tokens[ti].text);
            if (wv >= 0)
              arg_map[key] = wv;
          } else {
            std::println(stderr, "Line {}: unexpected token '{}'", lineno,
                         tokens[ti].text);
            return false;
          }
        }
      }

      for (int ai = 0; ai < info->arg_count; ai++) {
        auto it = arg_map.find(info->args[ai].name);
        if (it == arg_map.end()) {
          std::println(stderr, "Line {}: missing value for '{}'", lineno,
                       info->args[ai].name);
          return false;
        }
        int64_t v = it->second;
        switch (info->args[ai].type) {
        case ArgType::U8:
          out.push_back(static_cast<uint8_t>(v));
          break;
        case ArgType::I8:
          out.push_back(static_cast<uint8_t>(static_cast<int8_t>(v)));
          break;
        case ArgType::I16: {
          util::LittleEndian<int16_t> w = static_cast<int16_t>(v);
          auto *b = reinterpret_cast<const uint8_t *>(&w);
          out.insert(out.end(), b, b + 2);
          break;
        }
        case ArgType::U32: {
          util::LittleEndian<uint32_t> dw = static_cast<uint32_t>(v);
          auto *b = reinterpret_cast<const uint8_t *>(&dw);
          out.insert(out.end(), b, b + 4);
          break;
        }
        default:
          break;
        }
      }
    }
  }

  std::println("Assembled SCL: {} bytes", out.size());
  return write_file(out_file, out);
}

// ============================================================================
// Localized text catalog assembler
// ============================================================================

struct TextSourceEntry {
  std::string key;
  uint32_t id = 0;
  std::vector<uint8_t> value;
};

static void append_u32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 24));
}

static bool write_text_catalog(std::vector<TextSourceEntry> entries,
                               const char *out_file) {
  std::ranges::sort(entries, {}, &TextSourceEntry::id);
  std::vector<uint8_t> out = {'S', 'S', 'T', 'X'};
  append_u32(out, 2);
  append_u32(out, static_cast<uint32_t>(entries.size()));
  for (const auto &entry : entries) {
    append_u32(out, entry.id);
    append_u32(out, static_cast<uint32_t>(entry.value.size()));
    out.insert(out.end(), entry.value.begin(), entry.value.end());
  }

  std::println("Assembled text catalog: {} entries, {} bytes", entries.size(),
               out.size());
  return write_file(out_file, out);
}

static std::string_view trim_catalog_whitespace(std::string_view text) {
  constexpr std::string_view whitespace = " \t\r";
  const auto first = text.find_first_not_of(whitespace);
  if (first == std::string_view::npos)
    return {};
  const auto last = text.find_last_not_of(whitespace);
  return text.substr(first, last - first + 1);
}

static bool valid_catalog_key(std::string_view key) {
  return !key.empty() && std::ranges::all_of(key, [](unsigned char c) {
    return std::isalnum(c) || c == '_' || c == '-' || c == '.';
  });
}

static bool cmd_asm_text(const char *in_file, const char *out_file) {
  std::ifstream ifs(in_file);
  if (!ifs) {
    std::println(stderr, "Error: Cannot open '{}'", in_file);
    return false;
  }

  std::vector<TextSourceEntry> entries;
  std::unordered_set<std::string> keys;
  std::unordered_map<uint32_t, std::string> ids;
  std::string line;
  int lineno = 0;
  while (std::getline(ifs, line)) {
    lineno++;
    const int entry_lineno = lineno;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const auto trimmed = trim_catalog_whitespace(line);
    if (trimmed.empty() || trimmed.front() == ';')
      continue;

    std::string key;
    std::vector<uint8_t> value;
    const auto equals = line.find('=');
    if (equals != std::string::npos &&
        trim_catalog_whitespace(std::string_view(line).substr(equals + 1)) ==
            "\"\"\"") {
      const auto key_view =
          trim_catalog_whitespace(std::string_view(line).substr(0, equals));
      if (!valid_catalog_key(key_view)) {
        std::println(stderr, "Line {}: invalid text key '{}'", lineno,
                     key_view);
        return false;
      }
      key = key_view;

      std::string multiline;
      bool closed = false;
      bool first_line = true;
      while (std::getline(ifs, line)) {
        lineno++;
        if (!line.empty() && line.back() == '\r')
          line.pop_back();
        if (trim_catalog_whitespace(line) == "\"\"\"") {
          closed = true;
          break;
        }
        if (!first_line)
          multiline.push_back('\n');
        multiline += line;
        first_line = false;
      }
      if (!closed) {
        std::println(stderr, "Line {}: unterminated multiline string for '{}'",
                     lineno, key);
        return false;
      }
      value = unescape_string(multiline);
    } else {
      auto tokens = tokenize_line(line, lineno);
      if (tokens.empty())
        return false;
      if (tokens.size() != 2 || tokens[0].kind != TokenKind::Key ||
          tokens[1].kind != TokenKind::String) {
        std::println(stderr, "Line {}: expected key = \"text\" or key = \"\"\"",
                     lineno);
        return false;
      }
      key = tokens[0].text;
      if (!valid_catalog_key(key)) {
        std::println(stderr, "Line {}: invalid text key '{}'", lineno, key);
        return false;
      }
      value = unescape_string(tokens[1].text);
    }

    if (!keys.insert(key).second) {
      std::println(stderr, "Line {}: duplicate text key '{}'", entry_lineno,
                   key);
      return false;
    }
    const auto id = text_id_hash(key);
    if (const auto it = ids.find(id); it != ids.end()) {
      std::println(stderr, "Line {}: text ID collision between '{}' and '{}'",
                   entry_lineno, it->second, key);
      return false;
    }
    ids.emplace(id, key);
    entries.push_back(TextSourceEntry{
        .key = key,
        .id = id,
        .value = std::move(value),
    });
  }
  if (entries.empty())
    return false;
  return write_text_catalog(std::move(entries), out_file);
}

// ============================================================================
// ECL disassembler
// ============================================================================

static bool cmd_disasm_ecl(const char *in_file, const char *out_file) {
  auto data = read_file(in_file);
  if (data.empty())
    return false;
  if (data.size() < 4) {
    std::println(stderr, "Error: ECL file too small");
    return false;
  }

  uint32_t script_count = *util::ReadLittleAt<uint32_t>(data, 0);
  if (script_count == 0 || script_count > 256) {
    std::println(stderr, "Error: Invalid script count {}", script_count);
    return false;
  }

  size_t header_size = 4 + script_count * 4;
  if (data.size() < header_size) {
    std::println(stderr, "Error: ECL header truncated");
    return false;
  }

  std::vector<uint32_t> entry_offsets(script_count);
  for (uint32_t i = 0; i < script_count; i++)
    entry_offsets[i] = *util::ReadLittleAt<uint32_t>(data, 4 + i * 4);

  // Collect all label targets
  std::unordered_set<uint32_t> labels;
  for (uint32_t i = 0; i < script_count; i++) {
    uint32_t off = entry_offsets[i];
    if (off < data.size())
      labels.insert(off);
  }

  for (uint32_t si = 0; si < script_count; si++) {
    uint32_t pos = entry_offsets[si];
    std::unordered_set<uint32_t> visited;
    while (pos < data.size()) {
      if (visited.contains(pos))
        break;
      visited.insert(pos);
      uint8_t op = data[pos];
      int len = ecl_cmd_len[op];
      if (len <= 0 || pos + len > data.size())
        break;

      auto collect = [&](uint32_t t) {
        if (t < data.size() && t >= header_size)
          labels.insert(t);
      };
      switch (op) {
      case 0x02:
      case 0x04:
      case 0x09:
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 1));
        break;
      case 0x03: // LOOP
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 1));
        break;
      case 0x06:
      case 0x07:
      case 0x0A:
      case 0x0B:
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 5));
        break;
      case 0x08:
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 1));
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 5));
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 9));
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 13));
        break;
      case 0x0C:
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 1));
        break;
      case 0xAB:
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 1));
        break;
      case 0xBA:
      case 0xBB:
      case 0xBE:
        collect(*util::ReadLittleAt<uint32_t>(data, pos + 1));
        break;
      }
      if (op == 0x01)
        break;
      pos += len;
    }
  }

  // Map offsets to script indices
  std::unordered_map<uint32_t, std::vector<uint32_t>> offset_to_scripts;
  for (uint32_t i = 0; i < script_count; i++)
    offset_to_scripts[entry_offsets[i]].push_back(i);

  // Assign label names
  std::vector<uint32_t> label_list(labels.begin(), labels.end());
  std::sort(label_list.begin(), label_list.end());
  std::unordered_map<uint32_t, std::string> label_names;
  for (uint32_t off : label_list) {
    auto it = offset_to_scripts.find(off);
    if (it != offset_to_scripts.end()) {
      label_names[off] = std::format("@script_{}", it->second[0]);
    } else {
      label_names[off] = std::format("@label_{:04X}", off);
    }
  }

  // Emit: code blocks at exact original offsets using .org directives
  std::string out;
  out += std::format(".header {}\n", script_count);

  // Emit offset table directives
  for (uint32_t i = 0; i < script_count; i++)
    out += std::format(".offset {} 0x{:04X}\n", i, entry_offsets[i]);
  out += '\n';

  // Track processed regions
  size_t pos = header_size;
  while (pos < data.size()) {
    uint8_t op = data[pos];
    int len = ecl_cmd_len[op];

    // Emit .org and label(s)
    if (labels.contains(static_cast<uint32_t>(pos))) {
      out += std::format(".org 0x{:04X}\n", pos);

      auto oit = offset_to_scripts.find(static_cast<uint32_t>(pos));
      if (oit != offset_to_scripts.end()) {
        for (size_t ii = 0; ii < oit->second.size(); ii++) {
          out += std::format("@script_{}:", oit->second[ii]);
          if (ii > 0)
            out += "  ; shared";
          out += '\n';
        }
      } else {
        auto lit = label_names.find(static_cast<uint32_t>(pos));
        if (lit != label_names.end())
          out += std::format("{}:\n", lit->second);
        else
          out += std::format("@label_{:04X}:\n", pos);
      }
    }

    if (len > 0 && pos + len <= data.size()) {
      auto *info = ecl_op_info(op);
      out += "    ";
      if (info) {
        out += info->name;
        int off = 1;
        for (int ai = 0; ai < info->arg_count; ai++) {
          const auto &arg = info->args[ai];
          out += ' ';
          out += arg.name;
          out += '=';
          switch (arg.type) {
          case ArgType::U8:
            out += std::to_string(data[pos + off]);
            off += 1;
            break;
          case ArgType::I8:
            out += std::to_string(static_cast<int8_t>(data[pos + off]));
            off += 1;
            break;
          case ArgType::U16:
            out +=
                std::to_string(*util::ReadLittleAt<uint16_t>(data, pos + off));
            off += 2;
            break;
          case ArgType::I16:
            out +=
                std::to_string(*util::ReadLittleAt<int16_t>(data, pos + off));
            off += 2;
            break;
          case ArgType::U32:
            out +=
                std::to_string(*util::ReadLittleAt<uint32_t>(data, pos + off));
            off += 4;
            break;
          case ArgType::I32:
            out +=
                std::to_string(*util::ReadLittleAt<int32_t>(data, pos + off));
            off += 4;
            break;
          case ArgType::Label: {
            uint32_t target = *util::ReadLittleAt<uint32_t>(data, pos + off);
            if (target >= data.size() || target < header_size) {
              out += std::format("0x{:04X}", target);
            } else {
              auto lit2 = label_names.find(target);
              if (lit2 != label_names.end())
                out += lit2->second;
              else
                out += std::format("0x{:04X}", target);
            }
            off += 4;
            break;
          }
          case ArgType::Vector: {
            auto *vn = stivect_name(data[pos + off]);
            out += vn ? vn : std::to_string(data[pos + off]);
            off += 1;
            break;
          }
          default:
            break;
          }
        }
        if (op == 0x00) {
          uint32_t hp = *util::ReadLittleAt<uint32_t>(data, pos + 1);
          if (hp == 0)
            out += "  ; death marker";
        }
      } else {
        out += std::format("; .byte 0x{:02X}", op);
        for (int b = 1; b < len; b++)
          out += std::format(", 0x{:02X}", data[pos + b]);
      }
      out += '\n';
      pos += len;
    } else {
      out += std::format("    .byte 0x{:02X}\n", op);
      pos += 1;
    }
  }

  std::ofstream ofs(out_file);
  if (!ofs) {
    std::println(stderr, "Error: Cannot write '{}'", out_file);
    return false;
  }
  ofs << out;
  std::println("Disassembled ECL to '{}'", out_file);
  return true;
}

// ============================================================================
// ECL assembler
// ============================================================================

static bool cmd_asm_ecl(const char *in_file, const char *out_file) {
  std::ifstream ifs(in_file);
  if (!ifs) {
    std::println(stderr, "Error: Cannot open '{}'", in_file);
    return false;
  }

  std::unordered_map<std::string, uint8_t> mnem_to_op;
  for (int o = 0; o < 256; o++) {
    auto *inf = ecl_op_info(static_cast<uint8_t>(o));
    if (inf && inf->name)
      mnem_to_op[inf->name] = static_cast<uint8_t>(o);
  }

  int script_count = -1;
  std::vector<uint32_t> entry_offsets;
  std::vector<uint8_t> out;
  uint32_t current_pos = 0;
  std::unordered_map<std::string, uint32_t> label_map;

  // Prepass: collect all label positions from .org directives
  {
    uint32_t pp_pos = 0;
    std::ifstream pp_ifs(in_file);
    if (!pp_ifs) {
      std::println(stderr, "Error: cannot open '{}' for prepass", in_file);
      return false;
    }
    std::string pp_line;
    int pp_lineno = 0;
    while (std::getline(pp_ifs, pp_line)) {
      pp_lineno++;
      auto pp_tokens = tokenize_line(pp_line, pp_lineno);
      if (pp_tokens.empty())
        continue;
      if (pp_tokens[0].kind == TokenKind::Comment)
        continue;
      if (pp_tokens[0].kind == TokenKind::Directive) {
        if (pp_tokens[0].text == "org" && pp_tokens.size() >= 2 &&
            pp_tokens[1].kind == TokenKind::Number) {
          pp_pos = static_cast<uint32_t>(pp_tokens[1].numval);
        }
        continue;
      }
      if (pp_tokens[0].kind == TokenKind::Label) {
        label_map[pp_tokens[0].text] = pp_pos;
        pp_tokens.erase(pp_tokens.begin());
        // Estimate position after this line's instruction
        if (!pp_tokens.empty() && pp_tokens[0].kind == TokenKind::Mnemonic) {
          auto pp_it = mnem_to_op.find(pp_tokens[0].text);
          if (pp_it != mnem_to_op.end()) {
            auto *pp_inf = ecl_op_info(pp_it->second);
            if (pp_inf)
              pp_pos += pp_inf->length;
          }
        }
        continue;
      }
      if (pp_tokens[0].kind == TokenKind::Mnemonic) {
        auto pp_it = mnem_to_op.find(pp_tokens[0].text);
        if (pp_it != mnem_to_op.end()) {
          auto *pp_inf = ecl_op_info(pp_it->second);
          if (pp_inf)
            pp_pos += pp_inf->length;
          else
            pp_pos++;
        } else if (pp_tokens[0].text == ".byte") {
          for (size_t tbi = 1; tbi < pp_tokens.size(); tbi++)
            if (pp_tokens[tbi].kind == TokenKind::Number)
              pp_pos++;
        } else {
          pp_pos++;
        }
      }
    }
  }

  // Rewind input for main pass
  ifs.clear();
  ifs.seekg(0);

  std::string raw_line;
  int lineno = 0;

  auto ensure_size = [&](size_t need) {
    if (out.size() < need)
      out.resize(need, 0);
  };

  while (std::getline(ifs, raw_line)) {
    lineno++;
    auto tokens = tokenize_line(raw_line, lineno);
    if (tokens.empty())
      continue;
    if (tokens[0].kind == TokenKind::Comment)
      continue;

    // Handle directives
    if (tokens[0].kind == TokenKind::Directive) {
      std::string dir = tokens[0].text;
      if (dir == "header") {
        if (tokens.size() >= 2 && tokens[1].kind == TokenKind::Number)
          script_count = static_cast<int>(tokens[1].numval);
        continue;
      }
      if (dir == "offset") {
        if (tokens.size() >= 3 && tokens[1].kind == TokenKind::Number &&
            tokens[2].kind == TokenKind::Number) {
          int idx = static_cast<int>(tokens[1].numval);
          uint32_t val = static_cast<uint32_t>(tokens[2].numval);
          if (static_cast<int>(entry_offsets.size()) <= idx)
            entry_offsets.resize(idx + 1, 0);
          entry_offsets[idx] = val;
        }
        continue;
      }
      if (dir == "org") {
        if (tokens.size() >= 2 && tokens[1].kind == TokenKind::Number)
          current_pos = static_cast<uint32_t>(tokens[1].numval);
        continue;
      }
      continue;
    }

    // Record labels
    if (tokens[0].kind == TokenKind::Label) {
      label_map[tokens[0].text] = current_pos;
      tokens.erase(tokens.begin());
      if (tokens.empty() || tokens[0].kind == TokenKind::Comment)
        continue;
    }
    if (tokens.empty() || tokens[0].kind == TokenKind::Comment)
      continue;

    if (tokens[0].kind == TokenKind::Mnemonic) {
      std::string mnem = tokens[0].text;

      if (mnem == ".byte") {
        for (size_t ti = 1; ti < tokens.size(); ti++) {
          if (tokens[ti].kind == TokenKind::Number) {
            ensure_size(current_pos + 1);
            out[current_pos++] = static_cast<uint8_t>(tokens[ti].numval);
          }
        }
        continue;
      }

      auto it = mnem_to_op.find(mnem);
      if (it == mnem_to_op.end()) {
        std::println(stderr, "Line {}: unknown mnemonic '{}'", lineno, mnem);
        return false;
      }
      uint8_t op = it->second;
      auto *inf = ecl_op_info(op);
      ensure_size(current_pos + inf->length);

      out[current_pos] = op;
      int off = 1;

      // Parse operands
      std::unordered_map<std::string, int64_t> arg_vals;
      std::unordered_map<std::string, std::string> arg_labels;
      std::string pending_key;
      for (size_t ti = 1; ti < tokens.size(); ti++) {
        if (tokens[ti].kind == TokenKind::Key) {
          pending_key = tokens[ti].text;
        } else if (tokens[ti].kind == TokenKind::Number) {
          arg_vals[pending_key] = tokens[ti].numval;
          pending_key.clear();
        } else if (tokens[ti].kind == TokenKind::LabelRef) {
          arg_labels[pending_key] = tokens[ti].text;
          pending_key.clear();
        } else if (tokens[ti].kind == TokenKind::Mnemonic) {
          int sv = stivect_value(tokens[ti].text);
          if (sv >= 0)
            arg_vals[pending_key] = sv;
          pending_key.clear();
        }
      }

      for (int ai = 0; ai < inf->arg_count; ai++) {
        const auto &arg = inf->args[ai];
        int64_t v = 0;
        if (arg.type == ArgType::Label) {
          auto lit = arg_labels.find(arg.name);
          if (lit != arg_labels.end()) {
            std::string lname = lit->second;
            auto mit = label_map.find(lname);
            if (mit != label_map.end()) {
              v = mit->second;
            } else if (lname.starts_with("label_")) {
              v = std::strtoul(lname.c_str() + 6, nullptr, 16);
            } else if (lname.starts_with("script_")) {
              int si = std::atoi(lname.c_str() + 7);
              if (si >= 0 && si < static_cast<int>(entry_offsets.size()))
                v = entry_offsets[si];
            }
          } else {
            auto ait = arg_vals.find(arg.name);
            if (ait != arg_vals.end())
              v = ait->second;
          }
          util::LittleEndian<uint32_t> dw = static_cast<uint32_t>(v);
          std::memcpy(&out[current_pos + off], &dw, 4);
          off += 4;
          continue;
        }
        if (arg.type == ArgType::Vector) {
          auto ait = arg_vals.find(arg.name);
          if (ait != arg_vals.end())
            v = ait->second;
          out[current_pos + off] = static_cast<uint8_t>(v);
          off += 1;
          continue;
        }
        auto ait = arg_vals.find(arg.name);
        if (ait != arg_vals.end())
          v = ait->second;
        switch (arg.type) {
        case ArgType::U8:
          out[current_pos + off] = static_cast<uint8_t>(v);
          off += 1;
          break;
        case ArgType::I8:
          out[current_pos + off] = static_cast<uint8_t>(static_cast<int8_t>(v));
          off += 1;
          break;
        case ArgType::U16: {
          util::LittleEndian<uint16_t> w = static_cast<uint16_t>(v);
          std::memcpy(&out[current_pos + off], &w, 2);
          off += 2;
          break;
        }
        case ArgType::I16: {
          util::LittleEndian<int16_t> w = static_cast<int16_t>(v);
          std::memcpy(&out[current_pos + off], &w, 2);
          off += 2;
          break;
        }
        case ArgType::U32:
        case ArgType::I32: {
          util::LittleEndian<uint32_t> dw = static_cast<uint32_t>(v);
          std::memcpy(&out[current_pos + off], &dw, 4);
          off += 4;
          break;
        }
        default:
          break;
        }
      }
      current_pos += inf->length;
    }
  }

  if (script_count <= 0) {
    std::println(stderr, "Error: missing .header directive");
    return false;
  }
  if (static_cast<int>(entry_offsets.size()) < script_count) {
    std::println(stderr, "Error: not enough .offset directives");
    return false;
  }

  // Write header
  ensure_size(4 + script_count * 4);
  if (!util::WriteLittleAt<uint32_t>(out, 0,
                                     static_cast<uint32_t>(script_count))) {
    return false;
  }
  for (int si = 0; si < script_count; si++) {
    if (!util::WriteLittleAt<uint32_t>(out, 4 + si * 4, entry_offsets[si])) {
      return false;
    }
  }

  std::println("Assembled ECL: {} bytes", out.size());
  return write_file(out_file, out);
}

// ============================================================================
// Usage
// ============================================================================

static void print_usage() {
  std::println(stderr, R"(script_tool - ECL/SCL disassembler and assembler

Usage:
  script_tool disasm-scl <in_binary> <out_text>
      Disassemble SCL binary to human-readable text

  script_tool asm-scl <in_text> <out_binary>
      Assemble SCL text back to binary

  script_tool asm-text <in_text> <out_binary>
      Assemble a localized text catalog

  script_tool disasm-ecl <in_binary> <out_text>
      Disassemble ECL binary to human-readable text with labels

  script_tool asm-ecl <in_text> <out_binary>
      Assemble ECL text back to binary

The .header N directive in ECL text sets the number of script entries.
Labels use @name: syntax.  Jump/call operands use @name references.
SCL strings use C-style escapes (\xNN, \\, \", \n, \r, \t).
Text catalog entries use key = "text" for single-line values. Multiline values
use key = """ followed by the text and a closing """ on its own line.
)");
}

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  std::string_view mode = argv[1];

  if (mode == "disasm-scl") {
    if (argc != 4) {
      std::println(stderr,
                   "Usage: script_tool disasm-scl <in_binary> <out_text>");
      return 1;
    }
    return cmd_disasm_scl(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "asm-scl") {
    if (argc != 4) {
      std::println(stderr, "Usage: script_tool asm-scl <in_text> <out_binary>");
      return 1;
    }
    return cmd_asm_scl(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "asm-text" || mode == "asm-messages" || mode == "asm-ui" ||
      mode == "asm-music") {
    if (argc != 4) {
      std::println(stderr,
                   "Usage: script_tool asm-text <in_text> <out_binary>");
      return 1;
    }
    return cmd_asm_text(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "disasm-ecl") {
    if (argc != 4) {
      std::println(stderr,
                   "Usage: script_tool disasm-ecl <in_binary> <out_text>");
      return 1;
    }
    return cmd_disasm_ecl(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "asm-ecl") {
    if (argc != 4) {
      std::println(stderr, "Usage: script_tool asm-ecl <in_binary> <out_text>");
      return 1;
    }
    return cmd_asm_ecl(argv[2], argv[3]) ? 0 : 1;
  }

  std::println(stderr, "Unknown mode: '{}'", mode);
  print_usage();
  return 1;
}
