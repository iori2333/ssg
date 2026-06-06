/*
 *   SclVM — Stage Control Language virtual machine
 *
 *   Executes SCL bytecode to drive stage progression: enemy spawning,
 *   boss encounters, BGM changes, messages, and stage flow control.
 *
 *   SclVM holds a non-owning program counter into SCL bytecode owned
 *   by EnemyManager::scl_head. Lifetime is managed alongside ECL data
 *   in LoadStageData().
 */

#pragma once

#include "ecl/scl_commands.h"
#include <cstdint>
#include <optional>

class SclVM {
public:
  // scl_data points into Enemies.scl_head (BYTE_BUFFER_OWNED).
  // SclVM does NOT take ownership.
  explicit SclVM(const uint8_t *scl_data) : m_base(scl_data), m_pc(scl_data) {}

  // Execute until a blocking command (SCL_KEY, SCL_TIME, SCL_WAITEX).
  // Returns true if SCL finished (SCL_END reached).
  bool Execute();

  // Current program counter (for syncing with SCROLL's SCL_Now)
  const uint8_t *PC() const { return m_pc; }
  void SetPC(const uint8_t *pc) { m_pc = pc; }

  // --- Global instance (same pattern as EclVM) ---
  static void Init(const uint8_t *scl_data);
  static void Clear();
  static SclVM &Instance();
  static bool IsInitialized();

private:
  const uint8_t *m_base; // SCL data start
  const uint8_t *m_pc;   // Program counter (non-owning)

  // Command handlers
  void SpawnEnemy(const SclCmdEnemy &c);

  static std::optional<SclVM> s_instance;
};
