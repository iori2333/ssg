/*
 *   EclInterpreter — Enemy Control Language bytecode interpreter
 */

#include "ecl_interpreter.h"
#include "ECL_LEN.h"
#include "enemy_manager.h"
#include "game/endian.h"

// 後方互換 inline wrapper は enemy_manager.h で提供

void EclInterpreter::Execute(EnemyData& e) {
  parse_ECL(&e);
}

void EclInterpreter::CheckInterrupts(EnemyData& e) {
  CheckECLInterrupt(&e);
}

void EclInterpreter::InitInterrupts(EnemyData& e) {
  InitECLInterrupt(&e);
}

void EclInterpreter::LongJump(EnemyData& e, uint32_t ecl_id) {
  EnemyECL_LongJump(&e, ecl_id);
}
