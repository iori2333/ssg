///
/// EclProgram - validated, decoded enemy script program
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "ecl.h"

#include "effect/effect_types.h"
#include "stage/stage_visuals.h"

struct EclNoArguments {};

struct EclSetupArguments {
  uint32_t hp;
  uint32_t score;
};

struct EclJumpArguments {
  size_t target;
};

struct EclLoopArguments {
  size_t target;
  uint16_t count;
};

struct EclConditionalJumpArguments {
  size_t target;
  uint32_t value;
};

struct EclDifficultyJumpArguments {
  std::array<size_t, 4> targets;
};

struct EclSetInterruptArguments {
  size_t target;
  EclInterrupt interrupt;
  uint32_t threshold;
};

struct EclInterruptArguments {
  EclInterrupt interrupt;
};

struct EclDurationArguments {
  uint16_t frames;
};

struct EclRotateArguments {
  int8_t angle_delta;
  uint16_t frames;
};

struct EclLinearRotateArguments {
  int32_t velocity_x;
  int32_t velocity_y;
  int8_t angle_delta;
  uint16_t frames;
};

struct EclWaveArguments {
  int32_t velocity;
  uint8_t amplitude;
  int8_t angle_delta;
  uint16_t frames;
};

struct EclAxisMoveArguments {
  uint16_t coordinate;
  uint16_t frames;
};

struct EclPointMoveArguments {
  uint16_t x;
  uint16_t y;
  uint16_t frames;
};

struct EclAccelerationArguments {
  int8_t acceleration;
  uint16_t frames;
};

struct EclAccelerationPointArguments {
  int16_t x;
  int16_t y;
  int16_t velocity;
};

struct EclByteArguments {
  uint8_t value;
};

struct EclSignedByteArguments {
  int8_t value;
};

struct EclSignedWordArguments {
  int16_t value;
};

struct EclSignedDwordArguments {
  int32_t value;
};

struct EclPointArguments {
  int16_t x;
  int16_t y;
};

struct EclBytePairArguments {
  uint8_t first;
  uint8_t second;
};

struct EclSignedBytePairArguments {
  int8_t first;
  int8_t second;
};

struct EclByteSignedByteArguments {
  uint8_t first;
  int8_t second;
};

struct EclLongLaserArguments {
  uint8_t id;
  int8_t angle_delta = 0;
};

struct EclAnimationArguments {
  uint8_t pattern;
  int8_t speed;
};

struct EclSpawnEnemyArguments {
  int16_t offset_x;
  int16_t offset_y;
  std::optional<EclValue> angle_source;
  uint32_t script_id;
};

struct EclHitboxArguments {
  uint16_t width;
  uint16_t height;
};

struct EclScriptArguments {
  uint32_t script_id;
};

struct EclBossActionArguments {
  EclBossAction action;
};

struct EclBitLaserArguments {
  EclBitLaserCommand command;
};

struct EclCommandValueArguments {
  EclBitCommand command;
  int32_t value;
};

struct EclCircleEffectArguments {
  int16_t offset_x;
  int16_t offset_y;
  CircleEffectKind effect;
};

struct EclStage4EffectArguments {
  stage::Stage4RockCommand command;
};

struct EclMoveValueArguments {
  EclValue destination;
  EclValue source;
};

struct EclRegisterConstantArguments {
  EclValue destination;
  uint32_t value;
};

struct EclRegisterSignedConstantArguments {
  EclValue destination;
  int32_t value;
};

struct EclRegisterValueArguments {
  EclValue destination;
  EclValue source;
};

struct EclRegisterPairArguments {
  EclValue first;
  EclValue second;
};

struct EclRegisterArguments {
  EclValue destination;
};

using EclArguments = std::variant<
    EclNoArguments, EclSetupArguments, EclJumpArguments, EclLoopArguments,
    EclConditionalJumpArguments, EclDifficultyJumpArguments,
    EclSetInterruptArguments, EclInterruptArguments, EclDurationArguments,
    EclRotateArguments, EclLinearRotateArguments, EclWaveArguments,
    EclAxisMoveArguments, EclPointMoveArguments, EclAccelerationArguments,
    EclAccelerationPointArguments, EclByteArguments, EclSignedByteArguments,
    EclSignedWordArguments, EclSignedDwordArguments, EclPointArguments,
    EclBytePairArguments, EclSignedBytePairArguments,
    EclByteSignedByteArguments, EclLongLaserArguments, EclAnimationArguments,
    EclSpawnEnemyArguments, EclHitboxArguments, EclScriptArguments,
    EclBossActionArguments, EclBitLaserArguments, EclCommandValueArguments,
    EclCircleEffectArguments, EclStage4EffectArguments, EclMoveValueArguments,
    EclRegisterConstantArguments, EclRegisterSignedConstantArguments,
    EclRegisterValueArguments, EclRegisterPairArguments, EclRegisterArguments>;

class EclInstructionFactory;

class EclInstruction {
public:
  [[nodiscard]] EclOpcode Opcode() const { return opcode_; }

  template <typename Arguments>
  [[nodiscard]] const Arguments &ArgumentsAs() const {
    return std::get<Arguments>(arguments_);
  }

private:
  friend class EclInstructionFactory;

  EclInstruction(EclOpcode opcode, EclArguments arguments)
      : opcode_(opcode), arguments_(arguments) {}

  EclOpcode opcode_;
  EclArguments arguments_;
};

class EclProgram {
public:
  [[nodiscard]] static std::optional<EclProgram>
  Parse(std::span<const uint8_t> bytes);

  [[nodiscard]] std::optional<size_t> Entry(uint32_t script_id) const;
  [[nodiscard]] const EclInstruction *InstructionAt(size_t position) const;
  [[nodiscard]] const std::vector<EclInstruction> &Instructions() const {
    return instructions_;
  }
  [[nodiscard]] bool Empty() const { return instructions_.empty(); }

private:
  std::vector<EclInstruction> instructions_;
  std::vector<size_t> entries_;
};
