/*
 *   ECL.h — aggregate header for the ECL scripting system
 *
 *   Includes opcode constants (EclOp, EclReg, etc.) and the interrupt
 *   vector structure. For opcode constants only, include ecl_opcodes.h
 *   directly.
 */

#pragma once

#include "ecl/ecl_opcodes.h"
#include <cstdint>

//// 割り込みベクタ構造体 ////
struct InterruptVector {
  uint32_t vect; // 割り込みベクタ(0 なら無効)
  int value;     // 比較値
};
