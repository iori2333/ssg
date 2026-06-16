/*
 *   EclInterpreter — Enemy Control Language bytecode interpreter
 */

#include "ecl_interpreter.h"
#include "ECL_LEN.h"
#include "enemy_manager.h"
#include "game/endian.h"

// 後方互換 inline wrapper は enemy_manager.h で提供

void EclInterpreter::Execute(EnemyData& e) {
  Enemies.ParseECL(&e);
}

void EclInterpreter::CheckInterrupts(EnemyData& e) {
  Enemies.CheckECLInterrupt(&e);
}

void EclInterpreter::InitInterrupts(EnemyData& e) {
  Enemies.InitECLInterrupt(&e);
}

void EclInterpreter::LongJump(EnemyData& e, uint32_t ecl_id) {
  Enemies.ECL_LongJump(&e, ecl_id);
}
