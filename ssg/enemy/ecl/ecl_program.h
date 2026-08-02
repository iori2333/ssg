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
  int count;
};

struct EclConditionalJumpArguments {
  size_t target;
  int value;
};

struct EclDifficultyJumpArguments {
  std::array<size_t, 4> targets;
};

struct EclSetInterruptArguments {
  size_t target;
  EclInterrupt interrupt;
  int threshold;
};

struct EclInterruptArguments {
  EclInterrupt interrupt;
};

struct EclDurationArguments {
  int frames;
};

struct EclRotateArguments {
  int angle_delta;
  int frames;
};

struct EclLinearRotateArguments {
  int velocity_x;
  int velocity_y;
  int angle_delta;
  int frames;
};

struct EclWaveArguments {
  int velocity;
  int amplitude;
  int angle_delta;
  int frames;
};

struct EclAxisMoveArguments {
  int coordinate;
  int frames;
};

struct EclPointMoveArguments {
  int x;
  int y;
  int frames;
};

struct EclAccelerationArguments {
  int acceleration;
  int frames;
};

struct EclAccelerationPointArguments {
  int x;
  int y;
  int velocity;
};

struct EclByteArguments {
  int value;
};

struct EclSignedByteArguments {
  int value;
};

struct EclSignedWordArguments {
  int value;
};

struct EclSignedDwordArguments {
  int value;
};

struct EclPointArguments {
  int x;
  int y;
};

struct EclBytePairArguments {
  int first;
  int second;
};

struct EclSignedBytePairArguments {
  int first;
  int second;
};

struct EclByteSignedByteArguments {
  int first;
  int second;
};

struct EclLongLaserArguments {
  std::size_t id;
  int angle_delta = 0;
};

struct EclAnimationArguments {
  std::size_t pattern;
  int speed;
};

struct EclSpawnEnemyArguments {
  int offset_x;
  int offset_y;
  std::optional<EclValue> angle_source;
  uint32_t script_id;
};

struct EclHitboxArguments {
  int width;
  int height;
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
  int value;
};

struct EclCircleEffectArguments {
  int offset_x;
  int offset_y;
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
  int value;
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
