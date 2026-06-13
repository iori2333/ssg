/*
 *   EclInterpreter — Enemy Control Language bytecode interpreter
 */

#include "ecl_interpreter.h"
#include "ECL_LEN.h"
#include "game/endian.h"

// Forward declarations from ENEMY.cpp
extern void parse_ECL(EnemyData* e);
extern void CheckECLInterrupt(EnemyData* e);
extern void InitECLInterrupt(EnemyData* e);
extern void EnemyECL_LongJump(EnemyData* e, uint32_t ecl_id);

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
