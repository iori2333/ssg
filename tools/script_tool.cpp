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

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <istream>
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

namespace {

// ============================================================================
// ECL command length table (from ECL_LEN.h)
// ============================================================================

const std::array<uint8_t, 256> ecl_cmd_len = {
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

enum class ArgType : uint8_t {
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

class ArgList {
public:
  constexpr ArgList(std::initializer_list<ArgDesc> args) : args_{} {
    for (std::size_t i = 0; i < args.size(); ++i) {
      args_[i] = args.begin()[i];
    }
  }

  constexpr const ArgDesc &operator[](std::size_t i) const { return args_[i]; }
  constexpr ArgDesc &operator[](std::size_t i) { return args_[i]; }

private:
  std::array<ArgDesc, 4> args_{};
};

struct EclOpInfo {
  const char *name = nullptr;
  uint8_t length = 0;
  ArgList args;
  int arg_count = 0;
};

struct SclOpInfo {
  const char *name = nullptr;
  ArgList args;
  int arg_count = 0;
};

// ============================================================================
// ECL opcode info table
// ============================================================================

const EclOpInfo *ecl_op_info(uint8_t op) {
  // clang-format off
    switch (op) {
    case 0x00: { static const EclOpInfo i = {.name="SETUP",      .length=9,  .args={{.name="hp", .type=ArgType::U32}, {.name="score", .type=ArgType::U32}}, .arg_count=2}; return &i; }
    case 0x01: { static const EclOpInfo i = {.name="END",        .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x02: { static const EclOpInfo i = {.name="JMP",        .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    case 0x03: { static const EclOpInfo i = {.name="LOOP",       .length=7,  .args={{.name="jmp", .type=ArgType::Label}, {.name="count", .type=ArgType::U16}}, .arg_count=2}; return &i; }
    case 0x04: { static const EclOpInfo i = {.name="CALL",       .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    case 0x05: { static const EclOpInfo i = {.name="RET",        .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x06: { static const EclOpInfo i = {.name="JHPL",       .length=9,  .args={{.name="hp", .type=ArgType::U32}, {.name="jmp", .type=ArgType::Label}}, .arg_count=2}; return &i; }
    case 0x07: { static const EclOpInfo i = {.name="JHPS",       .length=9,  .args={{.name="hp", .type=ArgType::U32}, {.name="jmp", .type=ArgType::Label}}, .arg_count=2}; return &i; }
    case 0x08: { static const EclOpInfo i = {.name="JDIF",      .length=17,  .args={{.name="easy", .type=ArgType::Label}, {.name="norm", .type=ArgType::Label}, {.name="hard", .type=ArgType::Label}, {.name="luna", .type=ArgType::Label}}, .arg_count=4}; return &i; }
    case 0x09: { static const EclOpInfo i = {.name="JDSB",       .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    case 0x0A: { static const EclOpInfo i = {.name="JFCL",       .length=9,  .args={{.name="frame", .type=ArgType::U32}, {.name="jmp", .type=ArgType::Label}}, .arg_count=2}; return &i; }
    case 0x0B: { static const EclOpInfo i = {.name="JFCS",       .length=9,  .args={{.name="frame", .type=ArgType::U32}, {.name="jmp", .type=ArgType::Label}}, .arg_count=2}; return &i; }
    case 0x0C: { static const EclOpInfo i = {.name="STI",       .length=10,  .args={{.name="jmp", .type=ArgType::Label}, {.name="vector", .type=ArgType::Vector}, {.name="val", .type=ArgType::U32}}, .arg_count=3}; return &i; }
    case 0x0D: { static const EclOpInfo i = {.name="CLI",        .length=2,  .args={{.name="vector", .type=ArgType::Vector}}, .arg_count=1}; return &i; }
    case 0x10: { static const EclOpInfo i = {.name="NOP",        .length=3,  .args={{.name="count", .type=ArgType::U16}}, .arg_count=1}; return &i; }
    case 0x11: { static const EclOpInfo i = {.name="NOPSC",      .length=3,  .args={{.name="count", .type=ArgType::U16}}, .arg_count=1}; return &i; }
    case 0x12: { static const EclOpInfo i = {.name="MOV",        .length=3,  .args={{.name="count", .type=ArgType::U16}}, .arg_count=1}; return &i; }
    case 0x13: { static const EclOpInfo i = {.name="ROL",        .length=4,  .args={{.name="deg", .type=ArgType::I8}, {.name="count", .type=ArgType::U16}}, .arg_count=2}; return &i; }
    case 0x14: { static const EclOpInfo i = {.name="LROL",      .length=12,  .args={{.name="vx", .type=ArgType::I32}, {.name="vy", .type=ArgType::I32}, {.name="deg", .type=ArgType::I8}, {.name="count", .type=ArgType::U16}}, .arg_count=4}; return &i; }
    case 0x15: { static const EclOpInfo i = {.name="WAVX",       .length=9,  .args={{.name="vx", .type=ArgType::I32}, {.name="amp", .type=ArgType::U8}, {.name="vd", .type=ArgType::I8}, {.name="count", .type=ArgType::U16}}, .arg_count=4}; return &i; }
    case 0x16: { static const EclOpInfo i = {.name="WAVY",       .length=9,  .args={{.name="vy", .type=ArgType::I32}, {.name="amp", .type=ArgType::U8}, {.name="vd", .type=ArgType::I8}, {.name="count", .type=ArgType::U16}}, .arg_count=4}; return &i; }
    case 0x17: { static const EclOpInfo i = {.name="MXA",        .length=5,  .args={{.name="x", .type=ArgType::U16}, {.name="count", .type=ArgType::U16}}, .arg_count=2}; return &i; }
    case 0x18: { static const EclOpInfo i = {.name="MYA",        .length=5,  .args={{.name="y", .type=ArgType::U16}, {.name="count", .type=ArgType::U16}}, .arg_count=2}; return &i; }
    case 0x19: { static const EclOpInfo i = {.name="MXYA",       .length=7,  .args={{.name="x", .type=ArgType::U16}, {.name="y", .type=ArgType::U16}, {.name="count", .type=ArgType::U16}}, .arg_count=3}; return &i; }
    case 0x1A: { static const EclOpInfo i = {.name="MXS",        .length=3,  .args={{.name="count", .type=ArgType::U16}}, .arg_count=1}; return &i; }
    case 0x1B: { static const EclOpInfo i = {.name="MYS",        .length=3,  .args={{.name="count", .type=ArgType::U16}}, .arg_count=1}; return &i; }
    case 0x1C: { static const EclOpInfo i = {.name="MXYS",       .length=3,  .args={{.name="count", .type=ArgType::U16}}, .arg_count=1}; return &i; }
    case 0x1D: { static const EclOpInfo i = {.name="ACC",        .length=4,  .args={{.name="accel", .type=ArgType::I8}, {.name="count", .type=ArgType::U16}}, .arg_count=2}; return &i; }
    case 0x1E: { static const EclOpInfo i = {.name="ACCXYA",     .length=7,  .args={{.name="dx", .type=ArgType::I16}, {.name="dy", .type=ArgType::I16}, {.name="v", .type=ArgType::I16}}, .arg_count=3}; return &i; }
    case 0x1F: { static const EclOpInfo i = {.name="GRAX",       .length=2,  .args={{.name="gravity", .type=ArgType::I8}}, .arg_count=1}; return &i; }
    case 0x20: { static const EclOpInfo i = {.name="DEGA",       .length=2,  .args={{.name="angle", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x21: { static const EclOpInfo i = {.name="DEGR",       .length=2,  .args={{.name="angle", .type=ArgType::I8}}, .arg_count=1}; return &i; }
    case 0x22: { static const EclOpInfo i = {.name="DEGX",       .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x23: { static const EclOpInfo i = {.name="DEGS",       .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x24: { static const EclOpInfo i = {.name="SPDA",       .length=5,  .args={{.name="speed", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x25: { static const EclOpInfo i = {.name="SPDR",       .length=5,  .args={{.name="speed", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x26: { static const EclOpInfo i = {.name="XYA",        .length=5,  .args={{.name="x", .type=ArgType::I16}, {.name="y", .type=ArgType::I16}}, .arg_count=2}; return &i; }
    case 0x27: { static const EclOpInfo i = {.name="XYR",        .length=5,  .args={{.name="dx", .type=ArgType::I16}, {.name="dy", .type=ArgType::I16}}, .arg_count=2}; return &i; }
    case 0x28: { static const EclOpInfo i = {.name="DEGXU",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x29: { static const EclOpInfo i = {.name="DEGXD",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x2A: { static const EclOpInfo i = {.name="DEGEX",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x2B: { static const EclOpInfo i = {.name="XYS",        .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x2C: { static const EclOpInfo i = {.name="DEGX2",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x2D: { static const EclOpInfo i = {.name="XYRND",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x2E: { static const EclOpInfo i = {.name="XYL",        .length=3,  .args={{.name="len", .type=ArgType::I16}}, .arg_count=1}; return &i; }
    case 0x40: { static const EclOpInfo i = {.name="TAMA",       .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x41: { static const EclOpInfo i = {.name="TAUTO",      .length=2,  .args={{.name="interval", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x42: { static const EclOpInfo i = {.name="TXYR",       .length=5,  .args={{.name="dx", .type=ArgType::I16}, {.name="dy", .type=ArgType::I16}}, .arg_count=2}; return &i; }
    case 0x43: { static const EclOpInfo i = {.name="TCMD",       .length=2,  .args={{.name="cmd", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x44: { static const EclOpInfo i = {.name="TDEGA",      .length=3,  .args={{.name="angle", .type=ArgType::U8}, {.name="dw", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0x45: { static const EclOpInfo i = {.name="TDEGR",      .length=3,  .args={{.name="angle", .type=ArgType::I8}, {.name="dw", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0x46: { static const EclOpInfo i = {.name="TNUMA",      .length=3,  .args={{.name="n", .type=ArgType::U8}, {.name="ns", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0x47: { static const EclOpInfo i = {.name="TNUMR",      .length=3,  .args={{.name="n", .type=ArgType::I8}, {.name="ns", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0x48: { static const EclOpInfo i = {.name="TSPDA",      .length=3,  .args={{.name="v", .type=ArgType::U8}, {.name="a", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0x49: { static const EclOpInfo i = {.name="TSPDR",      .length=3,  .args={{.name="v", .type=ArgType::I8}, {.name="a", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0x4A: { static const EclOpInfo i = {.name="TOPT",       .length=2,  .args={{.name="opt", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x4B: { static const EclOpInfo i = {.name="TTYPE",      .length=2,  .args={{.name="type", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x4C: { static const EclOpInfo i = {.name="TCOL",       .length=2,  .args={{.name="color", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x4D: { static const EclOpInfo i = {.name="TVDEG",      .length=2,  .args={{.name="vd", .type=ArgType::I8}}, .arg_count=1}; return &i; }
    case 0x4E: { static const EclOpInfo i = {.name="TREP",       .length=2,  .args={{.name="rep", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x4F: { static const EclOpInfo i = {.name="TDEGS",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x50: { static const EclOpInfo i = {.name="TDEGE",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x51: { static const EclOpInfo i = {.name="TAMA2",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x52: { static const EclOpInfo i = {.name="TCLR",       .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x53: { static const EclOpInfo i = {.name="TAMAL",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x54: { static const EclOpInfo i = {.name="T2ITEM",     .length=2,  .args={{.name="pct", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x55: { static const EclOpInfo i = {.name="TAMAEX",     .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x60: { static const EclOpInfo i = {.name="LASER",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x61: { static const EclOpInfo i = {.name="LCMD",       .length=2,  .args={{.name="cmd", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x62: { static const EclOpInfo i = {.name="LLA",        .length=5,  .args={{.name="len", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x63: { static const EclOpInfo i = {.name="LLR",        .length=5,  .args={{.name="len", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x64: { static const EclOpInfo i = {.name="LL2",        .length=5,  .args={{.name="l2", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x65: { static const EclOpInfo i = {.name="LDEGA",      .length=3,  .args={{.name="angle", .type=ArgType::U8}, {.name="dw", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0x66: { static const EclOpInfo i = {.name="LDEGR",      .length=3,  .args={{.name="angle", .type=ArgType::I8}, {.name="dw", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0x67: { static const EclOpInfo i = {.name="LNUMA",      .length=2,  .args={{.name="n", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x68: { static const EclOpInfo i = {.name="LNUMR",      .length=2,  .args={{.name="n", .type=ArgType::I8}}, .arg_count=1}; return &i; }
    case 0x69: { static const EclOpInfo i = {.name="LSPDA",      .length=5,  .args={{.name="v", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x6A: { static const EclOpInfo i = {.name="LSPDR",      .length=5,  .args={{.name="v", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x6B: { static const EclOpInfo i = {.name="LCOL",       .length=2,  .args={{.name="color", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x6C: { static const EclOpInfo i = {.name="LTYPE",      .length=2,  .args={{.name="type", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x6D: { static const EclOpInfo i = {.name="LWA",        .length=5,  .args={{.name="w", .type=ArgType::I32}}, .arg_count=1}; return &i; }
    case 0x6E: { static const EclOpInfo i = {.name="LDEGS",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x6F: { static const EclOpInfo i = {.name="LDEGE",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x70: { static const EclOpInfo i = {.name="LXY",        .length=5,  .args={{.name="x", .type=ArgType::I16}, {.name="y", .type=ArgType::I16}}, .arg_count=2}; return &i; }
    case 0x71: { static const EclOpInfo i = {.name="LASER2",     .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x80: { static const EclOpInfo i = {.name="LLSET",      .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x81: { static const EclOpInfo i = {.name="LLOPEN",     .length=2,  .args={{.name="id", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x82: { static const EclOpInfo i = {.name="LLCLOSE",    .length=2,  .args={{.name="id", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x83: { static const EclOpInfo i = {.name="LLCLOSEL",   .length=2,  .args={{.name="id", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0x84: { static const EclOpInfo i = {.name="LLDEGR",     .length=3,  .args={{.name="id", .type=ArgType::U8}, {.name="deg", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0x85: { static const EclOpInfo i = {.name="HLASER",     .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x90: { static const EclOpInfo i = {.name="DRAW_ON",    .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x91: { static const EclOpInfo i = {.name="DRAW_OFF",   .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x92: { static const EclOpInfo i = {.name="CLIP_ON",    .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x93: { static const EclOpInfo i = {.name="CLIP_OFF",   .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x94: { static const EclOpInfo i = {.name="DAMAGE_ON",  .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x95: { static const EclOpInfo i = {.name="DAMAGE_OFF", .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x96: { static const EclOpInfo i = {.name="HITSB_ON",   .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x97: { static const EclOpInfo i = {.name="HITSB_OFF",  .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x98: { static const EclOpInfo i = {.name="RLCHG_ON",   .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0x99: { static const EclOpInfo i = {.name="RLCHG_OFF",  .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0xA0: { static const EclOpInfo i = {.name="ANM",        .length=3,  .args={{.name="pattern", .type=ArgType::U8}, {.name="speed", .type=ArgType::I8}}, .arg_count=2}; return &i; }
    case 0xA1: { static const EclOpInfo i = {.name="PSE",        .length=2,  .args={{.name="id", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xA2: { static const EclOpInfo i = {.name="INT",        .length=2,  .args={{.name="id", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xA3: { static const EclOpInfo i = {.name="EXDEGD",     .length=2,  .args={{.name="deg", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xA4: { static const EclOpInfo i = {.name="ENEMYSET",   .length=6,  .args={{.name="dx", .type=ArgType::I16}, {.name="dy", .type=ArgType::I16}, {.name="id", .type=ArgType::U8}}, .arg_count=3}; return &i; }
    case 0xA5: { static const EclOpInfo i = {.name="ENEMYSETD",  .length=7,  .args={{.name="dx", .type=ArgType::I16}, {.name="dy", .type=ArgType::I16}, {.name="reg", .type=ArgType::U8}, {.name="id", .type=ArgType::U8}}, .arg_count=4}; return &i; }
    case 0xA6: { static const EclOpInfo i = {.name="HITXY",      .length=5,  .args={{.name="w", .type=ArgType::U16}, {.name="h", .type=ArgType::U16}}, .arg_count=2}; return &i; }
    case 0xA7: { static const EclOpInfo i = {.name="ITEM",       .length=2,  .args={{.name="type", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xA8: { static const EclOpInfo i = {.name="STG4EFC",    .length=2,  .args={{.name="cmd", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xA9: { static const EclOpInfo i = {.name="ANMEX",      .length=2,  .args={{.name="pattern", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xAA: { static const EclOpInfo i = {.name="BITLASER",   .length=2,  .args={{.name="cmd", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xAB: { static const EclOpInfo i = {.name="BITATTACK",  .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    case 0xAC: { static const EclOpInfo i = {.name="BITCMD",     .length=6,  .args={{.name="cmd", .type=ArgType::U8}, {.name="val", .type=ArgType::I32}}, .arg_count=2}; return &i; }
    case 0xAD: { static const EclOpInfo i = {.name="BOSSSET",    .length=2,  .args={{.name="id", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xAE: { static const EclOpInfo i = {.name="CEFC",       .length=6,  .args={{.name="x", .type=ArgType::I16}, {.name="y", .type=ArgType::I16}, {.name="type", .type=ArgType::U8}}, .arg_count=3}; return &i; }
    case 0xAF: { static const EclOpInfo i = {.name="STG3EFC",    .length=1,  .args={}, .arg_count=0}; return &i; }
    case 0xB0: { static const EclOpInfo i = {.name="MOVR",       .length=3,  .args={{.name="dst", .type=ArgType::U8}, {.name="src", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0xB1: { static const EclOpInfo i = {.name="MOVC",       .length=6,  .args={{.name="dst", .type=ArgType::U8}, {.name="val", .type=ArgType::U32}}, .arg_count=2}; return &i; }
    case 0xB2: { static const EclOpInfo i = {.name="ADD",        .length=3,  .args={{.name="dst", .type=ArgType::U8}, {.name="src", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0xB3: { static const EclOpInfo i = {.name="SUB",        .length=3,  .args={{.name="dst", .type=ArgType::U8}, {.name="src", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0xB4: { static const EclOpInfo i = {.name="SINL",       .length=3,  .args={{.name="len", .type=ArgType::U8}, {.name="deg", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0xB5: { static const EclOpInfo i = {.name="COSL",       .length=3,  .args={{.name="len", .type=ArgType::U8}, {.name="deg", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0xB6: { static const EclOpInfo i = {.name="MOD",        .length=6,  .args={{.name="reg", .type=ArgType::U8}, {.name="div", .type=ArgType::U32}}, .arg_count=2}; return &i; }
    case 0xB7: { static const EclOpInfo i = {.name="RND",        .length=2,  .args={{.name="reg", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xB8: { static const EclOpInfo i = {.name="CMPR",       .length=3,  .args={{.name="reg0", .type=ArgType::U8}, {.name="reg1", .type=ArgType::U8}}, .arg_count=2}; return &i; }
    case 0xB9: { static const EclOpInfo i = {.name="CMPC",       .length=6,  .args={{.name="reg", .type=ArgType::U8}, {.name="val", .type=ArgType::U32}}, .arg_count=2}; return &i; }
    case 0xBA: { static const EclOpInfo i = {.name="JL",         .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    case 0xBB: { static const EclOpInfo i = {.name="JS",         .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    case 0xBC: { static const EclOpInfo i = {.name="INC",        .length=2,  .args={{.name="reg", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xBD: { static const EclOpInfo i = {.name="DEC",        .length=2,  .args={{.name="reg", .type=ArgType::U8}}, .arg_count=1}; return &i; }
    case 0xBE: { static const EclOpInfo i = {.name="JEQ",        .length=5,  .args={{.name="jmp", .type=ArgType::Label}}, .arg_count=1}; return &i; }
    default: return nullptr;
    }
  // clang-format on
}

// ============================================================================
// SCL opcode info table
// ============================================================================

const SclOpInfo *scl_op_info(uint8_t op) {
  static const std::array<SclOpInfo, 24> table = {{
      {.name = "TIME",
       .args = {{.name = "frame", .type = ArgType::U32}},
       .arg_count = 1},
      {.name = "ENEMY",
       .args = {{.name = "x", .type = ArgType::I16},
                {.name = "y", .type = ArgType::I16},
                {.name = "id", .type = ArgType::U8}},
       .arg_count = 3},
      {.name = "SSP",
       .args = {{.name = "speed", .type = ArgType::I16}},
       .arg_count = 1},
      {.name = "EFC",
       .args = {{.name = "type", .type = ArgType::U8}},
       .arg_count = 1},
      {.name = "END", .args = {}, .arg_count = 0},
      {.name = "BOSS",
       .args = {{.name = "x", .type = ArgType::I16},
                {.name = "y", .type = ArgType::I16},
                {.name = "id", .type = ArgType::U8}},
       .arg_count = 3},
      {.name = "MWOPEN", .args = {}, .arg_count = 0},
      {.name = "MWCLOSE", .args = {}, .arg_count = 0},
      {.name = "MSG",
       .args = {{.name = "text", .type = ArgType::String}},
       .arg_count = 1},
      {.name = "KEY", .args = {}, .arg_count = 0},
      {.name = "NPG", .args = {}, .arg_count = 0},
      {.name = "FACE",
       .args = {{.name = "id", .type = ArgType::U8}},
       .arg_count = 1},
      {.name = "MUSIC",
       .args = {{.name = "id", .type = ArgType::U8}},
       .arg_count = 1},
      {.name = "BOSSDEAD", .args = {}, .arg_count = 0},
      {.name = "LOADFACE",
       .args = {{.name = "surf", .type = ArgType::U8},
                {.name = "file", .type = ArgType::U8}},
       .arg_count = 2},
      {.name = "WAITEX",
       .args = {{.name = "cond", .type = ArgType::U8},
                {.name = "opt", .type = ArgType::U32}},
       .arg_count = 2},
      {.name = "STAGECLEAR", .args = {}, .arg_count = 0},
      {.name = "MAPPALETTE", .args = {}, .arg_count = 0},
      {.name = "GAMECLEAR", .args = {}, .arg_count = 0},
      {.name = "DELENEMY", .args = {}, .arg_count = 0},
      {.name = "ENEMYPALETTE", .args = {}, .arg_count = 0},
      {.name = "STAFF",
       .args = {{.name = "id", .type = ArgType::U8}},
       .arg_count = 1},
      {.name = "EXTRACLEAR", .args = {}, .arg_count = 0},
      {.name = "MSGREF",
       .args = {{.name = "id", .type = ArgType::U32}},
       .arg_count = 1},
  }};
  if (op <= 0x17 && (table[op].name != nullptr)) {
    return &table[op];
  }
  return nullptr;
}

// ============================================================================
// Symbolic name helpers
// ============================================================================

const char *stivect_name(uint8_t v) {
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

int stivect_value(std::string_view s) {
  if (s == "BOSSLEFT") {
    return 0;
  }
  if (s == "HP") {
    return 1;
  }
  if (s == "TIMER") {
    return 2;
  }
  if (s == "BITLEFT") {
    return 3;
  }
  return -1;
}

const char *efc_name(uint8_t t) {
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

int efc_value(std::string_view s) {
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

const char *scwait_name(uint8_t c) {
  switch (c) {
  case 0:
    return "BOSSLEFT";
  case 1:
    return "BOSSHP";
  default:
    return nullptr;
  }
}

int scwait_value(std::string_view s) {
  if (s == "BOSSLEFT") {
    return 0;
  }
  if (s == "BOSSHP") {
    return 1;
  }
  return -1;
}

// ============================================================================
// File I/O
// ============================================================================

std::vector<uint8_t> read_file(const char *path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::println(stderr, "Error: Cannot open '{}'", path);
    return {};
  }
  file.seekg(0, std::ios::end);
  const auto size = file.tellg();
  file.seekg(0, std::ios::beg);
  if (!file || size < 0) {
    std::println(stderr, "Error: Cannot size '{}'", path);
    return {};
  }
  std::vector<uint8_t> buf(static_cast<size_t>(size));
  if (size > 0) {
    file.read(reinterpret_cast<char *>(buf.data()),
              static_cast<std::streamsize>(size));
  }
  if (!file) {
    std::println(stderr, "Error: Failed to read '{}'", path);
    return {};
  }
  return buf;
}

bool write_file(const char *path, const std::vector<uint8_t> &data) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    std::println(stderr, "Error: Cannot write '{}'", path);
    return false;
  }
  if (!data.empty()) {
    file.write(reinterpret_cast<const char *>(data.data()),
               static_cast<std::streamsize>(data.size()));
  }
  if (!file) {
    std::println(stderr, "Error: Failed to write '{}'", path);
    return false;
  }
  file.close();
  if (!file) {
    std::println(stderr, "Error: Failed to close '{}'", path);
    return false;
  }
  return true;
}

// ============================================================================
// String escaping
// ============================================================================

std::string escape_string(const uint8_t *data, size_t len) {
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

std::vector<uint8_t> unescape_string(std::string_view s) {
  std::vector<uint8_t> out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      switch (s[i + 1]) {
      case 'x':
      case 'X': {
        if (i + 3 < s.size() &&
            (std::isxdigit(static_cast<unsigned char>(s[i + 2])) != 0) &&
            (std::isxdigit(static_cast<unsigned char>(s[i + 3])) != 0)) {
          std::array<char, 3> hex = {s[i + 2], s[i + 3], 0};
          char *end = nullptr;
          out.push_back(
              static_cast<uint8_t>(std::strtoul(hex.data(), &end, 16)));
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

uint32_t text_id_hash(std::string_view key) {
  return util::TextIdFromKey(key);
}

// ============================================================================
// Tokenizer for assembler input
// ============================================================================

enum class TokenKind : uint8_t {
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

std::vector<Token> tokenize_line(std::string_view line, int lineno) {
  std::vector<Token> tokens;
  size_t i = 0;
  while (i < line.size()) {
    char const c = line[i];
    if (c == ' ' || c == '\t' || c == '\r') {
      i++;
      continue;
    }
    if (c == ';') {
      break;
    }

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
      Token const t{.kind = TokenKind::String, .text = str, .numval = 0};
      tokens.push_back(t);
      continue;
    }

    if (c == '@') {
      size_t const start = i;
      i++;
      while (i < line.size() &&
             ((std::isalnum(static_cast<unsigned char>(line[i])) != 0) ||
              line[i] == '_')) {
        i++;
      }
      std::string const name(line.substr(start + 1, i - start - 1));
      if (name.empty()) {
        i = start + 1;
        continue;
      }
      bool const is_def = (i < line.size() && line[i] == ':');
      Token t;
      t.kind = is_def ? TokenKind::Label : TokenKind::LabelRef;
      t.text = name;
      if (is_def) {
        i++;
      }
      tokens.push_back(t);
      continue;
    }

    if (c == '.') {
      size_t const start = i;
      i++;
      while (i < line.size() &&
             (std::isspace(static_cast<unsigned char>(line[i])) == 0) &&
             line[i] != ';') {
        i++;
      }
      Token const t{.kind = TokenKind::Directive,
                    .text = std::string(line.substr(start + 1, i - start - 1)),
                    .numval = 0};
      tokens.push_back(t);
      continue;
    }

    if (c == '-' || c == '+' ||
        (std::isdigit(static_cast<unsigned char>(c)) != 0)) {
      size_t const start = i;
      bool const neg = (c == '-');
      if (c == '-' || c == '+') {
        i++;
      }
      int base = 10;
      if (i < line.size() && line[i] == '0' && i + 1 < line.size() &&
          (line[i + 1] == 'x' || line[i + 1] == 'X')) {
        i += 2;
        base = 16;
      }
      while (i < line.size() &&
             ((base == 16
                   ? std::isxdigit(static_cast<unsigned char>(line[i]))
                   : std::isdigit(static_cast<unsigned char>(line[i]))) != 0)) {
        i++;
      }
      std::string const numstr(line.substr(start, i - start));
      int64_t const v = std::strtoll(numstr.c_str(), nullptr, 0);
      Token const t{.kind = TokenKind::Number, .text = numstr, .numval = v};
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
      size_t const start = i;
      while (i < line.size() &&
             (std::isspace(static_cast<unsigned char>(line[i])) == 0) &&
             line[i] != ';' && line[i] != '=' && line[i] != '"' &&
             line[i] != '@') {
        i++;
      }
      std::string word(line.substr(start, i - start));
      if (word.empty()) {
        continue;
      }
      bool const has_eq = (word.back() == '=');
      if (has_eq) {
        word.pop_back();
      }

      if (tokens.empty() || tokens.back().kind == TokenKind::Label) {
        Token const t{.kind = TokenKind::Mnemonic, .text = word, .numval = 0};
        tokens.push_back(t);
        if (has_eq) {
          tokens.back().kind = TokenKind::Key;
        }
      } else if (tokens.back().kind == TokenKind::Mnemonic ||
                 tokens.back().kind == TokenKind::Key ||
                 tokens.back().kind == TokenKind::Number ||
                 tokens.back().kind == TokenKind::LabelRef ||
                 tokens.back().kind == TokenKind::String) {
        Token const t{.kind = has_eq ? TokenKind::Key : TokenKind::Mnemonic,
                      .text = word,
                      .numval = 0};
        tokens.push_back(t);
      } else {
        Token const t{.kind = TokenKind::Mnemonic, .text = word, .numval = 0};
        tokens.push_back(t);
      }
    }
  }
  if (tokens.empty()) {
    tokens.push_back({.kind = TokenKind::Comment, .text = {}, .numval = 0});
  }
  return tokens;
}

// ============================================================================
// SCL disassembler
// ============================================================================

bool cmd_disasm_scl(const char *in_file, const char *out_file) {
  auto data = read_file(in_file);
  if (data.empty()) {
    return false;
  }

  std::string out;
  size_t pos = 0;
  while (pos < data.size()) {
    uint8_t op = data[pos];
    out += "    ";
    const auto *info = scl_op_info(op);

    if (op == 0x08) {
      pos++;
      size_t const str_start = pos;
      while (pos < data.size() && data[pos] != 0) {
        pos++;
      }
      std::string escaped = escape_string(&data[str_start], pos - str_start);
      out += std::format("MSG \"{}\"\n", escaped);
      if (pos < data.size()) {
        pos++;
      }
    } else if (info != nullptr) {
      out += info->name;
      int off = 1;
      for (int ai = 0; ai < info->arg_count; ai++) {
        const auto &arg = info->args[ai];
        out += ' ';
        out += arg.name;
        out += '=';
        switch (arg.type) {
        case ArgType::U8: {
          uint8_t const v = data[pos + off];
          if (ai == 0 && op == 0x03) {
            const auto *ename = efc_name(v);
            out += (ename != nullptr) ? ename : std::to_string(v);
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
          uint32_t const v = *util::ReadLittleAt<uint32_t>(data, pos + off);
          if (ai == 0 && op == 0x0F) {
            const auto *cname = scwait_name(static_cast<uint8_t>(v));
            out += (cname != nullptr) ? cname : std::to_string(v);
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
      int op_len = 1;
      if (op == 0x00 || op == 0x17) {
        op_len = 5;
      } else if (op == 0x01 || op == 0x05 || op == 0x0F) {
        op_len = 6;
      } else if (op == 0x02 || op == 0x0E) {
        op_len = 3;
      } else if (op == 0x03 || op == 0x0B || op == 0x0C || op == 0x15) {
        op_len = 2;
      }
      pos += op_len;
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

bool cmd_asm_scl(const char *in_file, const char *out_file) {
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
    if (tokens.empty() || tokens[0].kind == TokenKind::Comment) {
      continue;
    }
    if (tokens[0].kind == TokenKind::Label ||
        tokens[0].kind == TokenKind::Directive) {
      continue;
    }

    if (tokens[0].kind != TokenKind::Mnemonic) {
      std::println(stderr, "Line {}: expected mnemonic", lineno);
      return false;
    }

    std::string mnem = tokens[0].text;
    uint8_t op = 0xFF;
    for (int o = 0; o <= 0x17; o++) {
      if (const auto *inf = scl_op_info(static_cast<uint8_t>(o))) {
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

    const auto *info = scl_op_info(op);
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
          std::string const key = pending_key.empty()
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
          std::string const key = pending_key.empty()
                                      ? info->args[arg_map.size()].name
                                      : pending_key;
          pending_key.clear();
          if (op == 0x03 && key == "type") {
            int const ev = efc_value(tokens[ti].text);
            if (ev >= 0) {
              arg_map[key] = ev;
            }
          } else if (op == 0x0F && key == "cond") {
            int const wv = scwait_value(tokens[ti].text);
            if (wv >= 0) {
              arg_map[key] = wv;
            }
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
          const auto *b = reinterpret_cast<const uint8_t *>(&w);
          out.insert(out.end(), b, b + 2);
          break;
        }
        case ArgType::U32: {
          util::LittleEndian<uint32_t> dw = static_cast<uint32_t>(v);
          const auto *b = reinterpret_cast<const uint8_t *>(&dw);
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

void append_u32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 24));
}

bool write_text_catalog(std::vector<TextSourceEntry> entries,
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

std::string_view trim_catalog_whitespace(std::string_view text) {
  constexpr std::string_view whitespace = " \t\r";
  const auto first = text.find_first_not_of(whitespace);
  if (first == std::string_view::npos) {
    return {};
  }
  const auto last = text.find_last_not_of(whitespace);
  return text.substr(first, last - first + 1);
}

bool valid_catalog_key(std::string_view key) {
  return !key.empty() && std::ranges::all_of(key, [](unsigned char c) {
    return std::isalnum(c) || c == '_' || c == '-' || c == '.';
  });
}

bool cmd_asm_text(const char *in_file, const char *out_file) {
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
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const auto trimmed = trim_catalog_whitespace(line);
    if (trimmed.empty() || trimmed.front() == ';') {
      continue;
    }

    std::string key;
    std::vector<uint8_t> value;
    const auto equals = line.find('=');
    if (equals != std::string::npos &&
        trim_catalog_whitespace(std::string_view(line).substr(equals + 1)) ==
            R"(""")") {
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
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }
        if (trim_catalog_whitespace(line) == R"(""")") {
          closed = true;
          break;
        }
        if (!first_line) {
          multiline.push_back('\n');
        }
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
      if (tokens.empty()) {
        return false;
      }
      if (tokens.size() != 2 || tokens[0].kind != TokenKind::Key ||
          tokens[1].kind != TokenKind::String) {
        std::println(stderr, R"(Line {}: expected key = "text" or key = """)",
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
  if (entries.empty()) {
    return false;
  }
  return write_text_catalog(std::move(entries), out_file);
}

// ============================================================================
// ECL disassembler
// ============================================================================

bool cmd_disasm_ecl(const char *in_file, const char *out_file) {
  auto data = read_file(in_file);
  if (data.empty()) {
    return false;
  }
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
  for (uint32_t i = 0; i < script_count; i++) {
    entry_offsets[i] = *util::ReadLittleAt<uint32_t>(data, 4 + i * 4);
  }

  // Collect all label targets
  std::unordered_set<uint32_t> labels;
  for (uint32_t i = 0; i < script_count; i++) {
    uint32_t const off = entry_offsets[i];
    if (off < data.size()) {
      labels.insert(off);
    }
  }

  for (uint32_t si = 0; si < script_count; si++) {
    uint32_t pos = entry_offsets[si];
    std::unordered_set<uint32_t> visited;
    while (pos < data.size()) {
      if (visited.contains(pos)) {
        break;
      }
      visited.insert(pos);
      uint8_t const op = data[pos];
      int const len = ecl_cmd_len[op];
      if (len <= 0 || pos + len > data.size()) {
        break;
      }

      auto collect = [&](uint32_t t) {
        if (t < data.size() && t >= header_size) {
          labels.insert(t);
        }
      };
      switch (op) {
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x09:
      case 0x0C:
      case 0xAB:
      case 0xBA:
      case 0xBB:
      case 0xBE:
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
      default:
        break;
      }
      if (op == 0x01) {
        break;
      }
      pos += len;
    }
  }

  // Map offsets to script indices
  std::unordered_map<uint32_t, std::vector<uint32_t>> offset_to_scripts;
  for (uint32_t i = 0; i < script_count; i++) {
    offset_to_scripts[entry_offsets[i]].push_back(i);
  }

  // Assign label names
  std::vector<uint32_t> label_list(labels.begin(), labels.end());
  std::ranges::sort(label_list);
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
  for (uint32_t i = 0; i < script_count; i++) {
    out += std::format(".offset {} 0x{:04X}\n", i, entry_offsets[i]);
  }
  out += '\n';

  // Track processed regions
  size_t pos = header_size;
  while (pos < data.size()) {
    uint8_t op = data[pos];
    int const len = ecl_cmd_len[op];

    // Emit .org and label(s)
    if (labels.contains(static_cast<uint32_t>(pos))) {
      out += std::format(".org 0x{:04X}\n", pos);

      auto oit = offset_to_scripts.find(static_cast<uint32_t>(pos));
      if (oit != offset_to_scripts.end()) {
        for (size_t ii = 0; ii < oit->second.size(); ii++) {
          out += std::format("@script_{}:", oit->second[ii]);
          if (ii > 0) {
            out += "  ; shared";
          }
          out += '\n';
        }
      } else {
        auto lit = label_names.find(static_cast<uint32_t>(pos));
        if (lit != label_names.end()) {
          out += std::format("{}:\n", lit->second);
        } else {
          out += std::format("@label_{:04X}:\n", pos);
        }
      }
    }

    if (len > 0 && pos + len <= data.size()) {
      const auto *info = ecl_op_info(op);
      out += "    ";
      if (info != nullptr) {
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
              if (lit2 != label_names.end()) {
                out += lit2->second;
              } else {
                out += std::format("0x{:04X}", target);
              }
            }
            off += 4;
            break;
          }
          case ArgType::Vector: {
            const auto *vn = stivect_name(data[pos + off]);
            out += (vn != nullptr) ? vn : std::to_string(data[pos + off]);
            off += 1;
            break;
          }
          default:
            break;
          }
        }
        if (op == 0x00) {
          uint32_t const hp = *util::ReadLittleAt<uint32_t>(data, pos + 1);
          if (hp == 0) {
            out += "  ; death marker";
          }
        }
      } else {
        out += std::format("; .byte 0x{:02X}", op);
        for (int b = 1; b < len; b++) {
          out += std::format(", 0x{:02X}", data[pos + b]);
        }
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

bool cmd_asm_ecl(const char *in_file, const char *out_file) {
  std::ifstream ifs(in_file);
  if (!ifs) {
    std::println(stderr, "Error: Cannot open '{}'", in_file);
    return false;
  }

  std::unordered_map<std::string, uint8_t> mnem_to_op;
  for (int o = 0; o < 256; o++) {
    const auto *inf = ecl_op_info(static_cast<uint8_t>(o));
    if ((inf != nullptr) && (inf->name != nullptr)) {
      mnem_to_op[inf->name] = static_cast<uint8_t>(o);
    }
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
      if (pp_tokens.empty()) {
        continue;
      }
      if (pp_tokens[0].kind == TokenKind::Comment) {
        continue;
      }
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
            const auto *pp_inf = ecl_op_info(pp_it->second);
            if (pp_inf != nullptr) {
              pp_pos += pp_inf->length;
            }
          }
        }
        continue;
      }
      if (pp_tokens[0].kind == TokenKind::Mnemonic) {
        auto pp_it = mnem_to_op.find(pp_tokens[0].text);
        if (pp_it != mnem_to_op.end()) {
          const auto *pp_inf = ecl_op_info(pp_it->second);
          if (pp_inf != nullptr) {
            pp_pos += pp_inf->length;
          } else {
            pp_pos++;
          }
        } else if (pp_tokens[0].text == ".byte") {
          for (size_t tbi = 1; tbi < pp_tokens.size(); tbi++) {
            if (pp_tokens[tbi].kind == TokenKind::Number) {
              pp_pos++;
            }
          }
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
    if (out.size() < need) {
      out.resize(need, 0);
    }
  };

  while (std::getline(ifs, raw_line)) {
    lineno++;
    auto tokens = tokenize_line(raw_line, lineno);
    if (tokens.empty()) {
      continue;
    }
    if (tokens[0].kind == TokenKind::Comment) {
      continue;
    }

    // Handle directives
    if (tokens[0].kind == TokenKind::Directive) {
      std::string const dir = tokens[0].text;
      if (dir == "header") {
        if (tokens.size() >= 2 && tokens[1].kind == TokenKind::Number) {
          script_count = static_cast<int>(tokens[1].numval);
        }
        continue;
      }
      if (dir == "offset") {
        if (tokens.size() >= 3 && tokens[1].kind == TokenKind::Number &&
            tokens[2].kind == TokenKind::Number) {
          int const idx = static_cast<int>(tokens[1].numval);
          auto const val = static_cast<uint32_t>(tokens[2].numval);
          if (std::cmp_less_equal(entry_offsets.size(), idx)) {
            entry_offsets.resize(idx + 1, 0);
          }
          entry_offsets[idx] = val;
        }
        continue;
      }
      if (dir == "org") {
        if (tokens.size() >= 2 && tokens[1].kind == TokenKind::Number) {
          current_pos = static_cast<uint32_t>(tokens[1].numval);
        }
        continue;
      }
      continue;
    }

    // Record labels
    if (tokens[0].kind == TokenKind::Label) {
      label_map[tokens[0].text] = current_pos;
      tokens.erase(tokens.begin());
      if (tokens.empty() || tokens[0].kind == TokenKind::Comment) {
        continue;
      }
    }
    if (tokens.empty() || tokens[0].kind == TokenKind::Comment) {
      continue;
    }

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
      uint8_t const op = it->second;
      const auto *inf = ecl_op_info(op);
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
          int const sv = stivect_value(tokens[ti].text);
          if (sv >= 0) {
            arg_vals[pending_key] = sv;
          }
          pending_key.clear();
        }
      }

      for (int ai = 0; ai < inf->arg_count; ai++) {
        const auto &arg = inf->args[ai];
        int64_t v = 0;
        if (arg.type == ArgType::Label) {
          auto lit = arg_labels.find(arg.name);
          if (lit != arg_labels.end()) {
            std::string const lname = lit->second;
            auto mit = label_map.find(lname);
            if (mit != label_map.end()) {
              v = mit->second;
            } else if (lname.starts_with("label_")) {
              v = std::strtoul(lname.c_str() + 6, nullptr, 16);
            } else if (lname.starts_with("script_")) {
              const char *start = lname.c_str() + 7;
              char *end = nullptr;
              errno = 0;
              const long parsed = std::strtol(start, &end, 10);
              if (errno == 0 && end != start && *end == '\0' && parsed >= 0 &&
                  std::cmp_less(parsed, entry_offsets.size())) {
                v = entry_offsets[static_cast<size_t>(parsed)];
              }
            }
          } else {
            auto ait = arg_vals.find(arg.name);
            if (ait != arg_vals.end()) {
              v = ait->second;
            }
          }
          util::LittleEndian<uint32_t> dw = static_cast<uint32_t>(v);
          std::memcpy(&out[current_pos + off], &dw, 4);
          off += 4;
          continue;
        }
        if (arg.type == ArgType::Vector) {
          auto ait = arg_vals.find(arg.name);
          if (ait != arg_vals.end()) {
            v = ait->second;
          }
          out[current_pos + off] = static_cast<uint8_t>(v);
          off += 1;
          continue;
        }
        auto ait = arg_vals.find(arg.name);
        if (ait != arg_vals.end()) {
          v = ait->second;
        }
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
  if (std::cmp_less(entry_offsets.size(), script_count)) {
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

void print_usage() {
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

} // namespace

// ============================================================================
// Entry point
// ============================================================================

namespace {
int MainImpl(int argc, char **argv) {
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
} // namespace

int main(int argc, char **argv) {
  try {
    return MainImpl(argc, argv);
  } catch (...) {
    return 1;
  }
}
