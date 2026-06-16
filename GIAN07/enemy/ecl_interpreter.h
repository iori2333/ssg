/*
 *   EclInterpreter — Enemy Control Language bytecode interpreter
 */

#pragma once

#include "ENEMY.h"
#include <cstdint>
#include <span>

class EclInterpreter {
public:
  explicit EclInterpreter(std::span<const uint8_t> ecl_data)
      : m_ecl_data(ecl_data) {}

  // Execute ECL bytecode for a single enemy
  void Execute(EnemyData& e);          // parse_ECL()

  // Interrupt vector management
  void CheckInterrupts(EnemyData& e);  // CheckECLInterrupt()
  void InitInterrupts(EnemyData& e);   // InitECLInterrupt()

  // Long jump to a different ECL block
  void LongJump(EnemyData& e, uint32_t ecl_id); // EnemyECL_LongJump()

  // Raw bytecode access
  std::span<const uint8_t> Data() const { return m_ecl_data; }

private:
  std::span<const uint8_t> m_ecl_data;
};
