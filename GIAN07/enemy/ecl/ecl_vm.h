///
/// EclVm - typed, cursor-based enemy script execution
///

#pragma once

#include <cstdint>
#include <utility>

#include "ecl_program.h"

#include "enemy/actor/enemy_actor.h"

class EclHost;
class EffectManager;

namespace audio {
class AudioSystem;
}

class EclVm {
public:
  EclVm(EclHost &host, EffectManager &effects, audio::AudioSystem &audio)
      : host_(host), effects_(effects), audio_(audio) {}

  void Install(EclProgram program) { program_ = std::move(program); }
  [[nodiscard]] bool Start(EnemyActor &actor, uint32_t script_id) const;
  [[nodiscard]] bool Jump(EnemyActor &actor, uint32_t script_id) const;
  void Execute(EnemyActor &actor);
  void CheckInterrupts(EnemyActor &actor);

private:
  enum class Step : uint8_t { Advance, Jump, Yield, Repeat, Halt };

  Step ExecuteInstruction(EnemyActor &actor, const EclInstruction &instruction,
                          int &comparison);
  Step ExecuteControlInstruction(EnemyActor &actor,
                                 const EclInstruction &instruction);
  Step ExecuteMovementInstruction(EnemyActor &actor,
                                  const EclInstruction &instruction);
  Step ExecuteBulletInstruction(EnemyActor &actor,
                                const EclInstruction &instruction);
  Step ExecuteLaserInstruction(EnemyActor &actor,
                               const EclInstruction &instruction);
  Step ExecuteActorInstruction(EnemyActor &actor,
                               const EclInstruction &instruction);
  static Step ExecuteRegisterInstruction(EnemyActor &actor,
                                         const EclInstruction &instruction,
                                         int &comparison);

  [[nodiscard]] static uint32_t ReadValue(const EnemyActor &actor,
                                          EclValue value);
  static void WriteValue(EnemyActor &actor, EclValue destination,
                         uint32_t value);

  EclProgram program_;
  EclHost &host_;
  EffectManager &effects_;
  audio::AudioSystem &audio_;
  uint8_t sequence_angle_ = 0;
  uint8_t sequence_angle_delta_ = 0;
};
