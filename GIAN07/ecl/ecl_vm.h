/*
 *   EclVM — Enemy Control Language bytecode virtual machine
 *
 *   Executes ECL bytecode instructions against EnemyData entities.
 *   Uses typed command structs (ecl_commands.h) for instruction decode.
 */

#pragma once

#include "ecl/ecl_opcodes.h"
#include <cstdint>
#include <optional>
#include <span>

struct EnemyData;

class EclVM {
public:
  explicit EclVM(std::span<const uint8_t> ecl_data) : m_ecl_data(ecl_data) {}

  // Execute ECL bytecode for a single enemy (main VM loop)
  void Execute(EnemyData &e);

  // Interrupt vector management
  static void CheckInterrupts(EnemyData &e);
  static void InitInterrupts(EnemyData &e);

  // Long jump to a different ECL block
  void LongJump(EnemyData &e, uint32_t ecl_id);

  // Raw bytecode access
  std::span<const uint8_t> Data() const { return m_ecl_data; }

  // --- Global instance management ---
  static void Init(std::span<const uint8_t> ecl_data);
  static void Clear();
  static EclVM &Instance();
  static bool IsInitialized();

private:
  std::span<const uint8_t> m_ecl_data;

  // Resolve an EclReg register/field ID to its value
  static uint32_t ResolveValue(const EnemyData *e, EclReg id);

  static std::optional<EclVM> s_instance;
};
