///
/// ECL runtime - typed instruction dispatch and per-frame execution
///

#include <algorithm>
#include <bit>
#include <utility>
#include <variant>

#include "ecl_host.h"
#include "ecl_program.h"
#include "ecl_vm.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "bullet/bullet_common.h"
#include "bullet/bullet_manager.h"
#include "effect/effect_manager.h"
#include "enemy/actor/enemy_actor.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "item/item_system.h"
#include "player/player.h"
#include "stage/stage_session.h"
#include "util/math_utils.h"

namespace {

template <typename Arguments>
const Arguments &Args(const EclInstruction &instruction) {
  return instruction.ArgumentsAs<Arguments>();
}

size_t RegisterIndex(EclValue value) { return static_cast<size_t>(value); }

} // namespace

bool EclVm::Start(EnemyActor &actor, uint32_t script_id) const {
  const auto entry = program_.Entry(script_id);
  if (!entry) {
    return false;
  }
  actor.script = {};
  actor.script.position = *entry;
  actor.script.return_position = *entry;
  return true;
}

bool EclVm::Jump(EnemyActor &actor, uint32_t script_id) const {
  const auto entry = program_.Entry(script_id);
  if (!entry) {
    return false;
  }
  actor.script.position = *entry;
  actor.script.return_position = *entry;
  actor.auto_fire_interval = 0;
  actor.script.loop_counter = 0;
  actor.script.wait_counter = 0;
  return true;
}

void EclVm::Execute(EnemyActor &actor) {
  constexpr uint32_t instruction_budget = 4096;
  int comparison = 0;
  for (uint32_t executed = 0; executed < instruction_budget; ++executed) {
    const auto *instruction = program_.InstructionAt(actor.script.position);
    if (instruction == nullptr) {
      actor.state = EnemyActorState::PendingRemoval;
      return;
    }
    switch (ExecuteInstruction(actor, *instruction, comparison)) {
    case Step::Advance:
      ++actor.script.position;
      break;
    case Step::Jump:
      break;
    case Step::Yield:
      ++actor.script.position;
      return;
    case Step::Repeat:
    case Step::Halt:
      return;
    }
  }
  actor.state = EnemyActorState::PendingRemoval;
}

EclVm::Step EclVm::ExecuteInstruction(EnemyActor &actor,
                                      const EclInstruction &instruction,
                                      int &comparison) {
  const auto opcode = std::to_underlying(instruction.Opcode());
  if (opcode <= std::to_underlying(EclOpcode::ClearInterrupt)) {
    return ExecuteControlInstruction(actor, instruction);
  }
  if (opcode >= std::to_underlying(EclOpcode::Wait) &&
      opcode <= std::to_underlying(EclOpcode::MovePolar)) {
    return ExecuteMovementInstruction(actor, instruction);
  }
  if (opcode >= std::to_underlying(EclOpcode::FireBullet) &&
      opcode <= std::to_underlying(EclOpcode::FireExtraBullet)) {
    return ExecuteBulletInstruction(actor, instruction);
  }
  if (opcode >= std::to_underlying(EclOpcode::FireLaser) &&
      opcode <= std::to_underlying(EclOpcode::FireHomingLaser)) {
    return ExecuteLaserInstruction(actor, instruction);
  }
  if (opcode >= std::to_underlying(EclOpcode::EnableDraw) &&
      opcode <= std::to_underlying(EclOpcode::Stage3Effect)) {
    return ExecuteActorInstruction(actor, instruction);
  }
  if (opcode >= std::to_underlying(EclOpcode::MoveValue) &&
      opcode <= std::to_underlying(EclOpcode::JumpEqual)) {
    return ExecuteRegisterInstruction(actor, instruction, comparison);
  }
  return Step::Halt;
}

EclVm::Step
EclVm::ExecuteControlInstruction(EnemyActor &actor,
                                 const EclInstruction &instruction) {
  switch (instruction.Opcode()) {
  case EclOpcode::Setup: {
    const auto &args = Args<EclSetupArguments>(instruction);
    actor.hp = args.hp;
    actor.score = args.score;
    if (actor.hp == 0) {
      host_.KillBosses();
    }
    return Step::Advance;
  }
  case EclOpcode::End:
    if (actor.long_laser_count != 0U) {
      host_.Bullets().ControlLongLaser(
          &actor, kEclAllLongLasers,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
    }
    actor.state = EnemyActorState::PendingRemoval;
    return Step::Halt;

  case EclOpcode::Jump:
    actor.script.position = Args<EclJumpArguments>(instruction).target;
    return Step::Jump;

  case EclOpcode::Loop: {
    const auto &args = Args<EclLoopArguments>(instruction);
    if (actor.script.loop_counter == 0) {
      actor.script.loop_counter = args.count + 1;
    }
    if (--actor.script.loop_counter != 0) {
      actor.script.position = args.target;
      return Step::Jump;
    }
    return Step::Advance;
  }

  case EclOpcode::Call:
    actor.script.return_position = actor.script.position + 1;
    actor.script.position = Args<EclJumpArguments>(instruction).target;
    return Step::Jump;

  case EclOpcode::Return:
    actor.script.position = actor.script.return_position;
    return Step::Jump;

  case EclOpcode::JumpHpGreater:
  case EclOpcode::JumpHpLess:
  case EclOpcode::JumpFrameGreater:
  case EclOpcode::JumpFrameLess: {
    const auto &args = Args<EclConditionalJumpArguments>(instruction);
    bool take_branch = false;
    switch (instruction.Opcode()) {
    case EclOpcode::JumpHpGreater:
      take_branch = actor.hp > args.value;
      break;
    case EclOpcode::JumpHpLess:
      take_branch = actor.hp < args.value;
      break;
    case EclOpcode::JumpFrameGreater:
      take_branch = actor.count > args.value;
      break;
    case EclOpcode::JumpFrameLess:
      take_branch = actor.count < args.value;
      break;
    default:
      break;
    }
    if (take_branch) {
      actor.script.position = args.target;
      return Step::Jump;
    }
    return Step::Advance;
  }

  case EclOpcode::JumpDifficulty: {
    const auto &targets = Args<EclDifficultyJumpArguments>(instruction).targets;
    switch (host_.Session().EffectiveLevel()) {
    case GameLevel::Easy:
      actor.script.position = targets[0];
      break;
    default:
    case GameLevel::Normal:
      actor.script.position = targets[1];
      break;
    case GameLevel::Hard:
      actor.script.position = targets[2];
      break;
    case GameLevel::Lunatic:
      actor.script.position = targets[3];
      break;
    }
    return Step::Jump;
  }

  case EclOpcode::JumpDirection: {
    const auto target =
        math::AngleTo(static_cast<float>(host_.GetPlayer().X() - actor.x),
                      static_cast<float>(host_.GetPlayer().Y() - actor.y));
    const auto difference = std::abs(
        math::ShortestAngleDelta(target, math::AngleFromLegacy(actor.d)));
    if (difference < 4.0f * math::kLegacyAngleStep) {
      actor.script.position = Args<EclJumpArguments>(instruction).target;
      return Step::Jump;
    }
    return Step::Advance;
  }

  case EclOpcode::SetInterrupt: {
    const auto &args = Args<EclSetInterruptArguments>(instruction);
    auto &interrupt =
        actor.script.interrupts[static_cast<size_t>(args.interrupt)];
    interrupt.target = args.target;
    interrupt.threshold = args.threshold;
    if (args.interrupt == EclInterrupt::Timer) {
      actor.script.interrupt_timer = 0;
    }
    return Step::Advance;
  }

  case EclOpcode::ClearInterrupt: {
    const auto interrupt = Args<EclInterruptArguments>(instruction).interrupt;
    actor.script.interrupts[static_cast<size_t>(interrupt)].target.reset();
    return Step::Advance;
  }

  default:
    return Step::Halt;
  }
}

EclVm::Step
EclVm::ExecuteMovementInstruction(EnemyActor &actor,
                                  const EclInstruction &instruction) {
  const auto absolute_angle = [&actor](uint8_t angle) -> uint8_t {
    return actor.HasFlag(EnemyActorFlags::HorizontalMirror) ? (128 - angle)
                                                            : angle;
  };
  const auto absolute_velocity_x = [&actor](int velocity) {
    return actor.HasFlag(EnemyActorFlags::HorizontalMirror) ? -velocity
                                                            : velocity;
  };
  const auto relative_angle = [&actor](int8_t angle) -> int8_t {
    return actor.HasFlag(EnemyActorFlags::HorizontalMirror)
               ? static_cast<int8_t>(-angle)
               : angle;
  };
  const auto continue_duration = [&actor](uint16_t frames) {
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = frames + 1;
    }
    return --actor.script.wait_counter != 0;
  };

  switch (instruction.Opcode()) {
  case EclOpcode::Wait:
  case EclOpcode::WaitScroll:
    return continue_duration(Args<EclDurationArguments>(instruction).frames)
               ? Step::Repeat
               : Step::Advance;

  case EclOpcode::Move: {
    const auto frames = Args<EclDurationArguments>(instruction).frames;
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = frames + 1;
      const auto velocity =
          math::RoundedPolarVector(math::AngleFromLegacy(actor.d), actor.v);
      actor.vx = velocity.x;
      actor.vy = velocity.y;
    }
    if (--actor.script.wait_counter != 0) {
      actor.x += actor.vx;
      actor.y += actor.vy;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::Rotate: {
    const auto &args = Args<EclRotateArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      actor.vd = relative_angle(args.angle_delta);
    }
    if (--actor.script.wait_counter != 0) {
      const auto velocity =
          math::RoundedPolarVector(math::AngleFromLegacy(actor.d), actor.v);
      actor.x += velocity.x;
      actor.y += velocity.y;
      actor.d += actor.vd;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::LinearRotate: {
    const auto &args = Args<EclLinearRotateArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      actor.vx = absolute_velocity_x(args.velocity_x);
      actor.vy = args.velocity_y;
      actor.vd = relative_angle(args.angle_delta);
    }
    if (--actor.script.wait_counter != 0) {
      const auto velocity =
          math::RoundedPolarVector(math::AngleFromLegacy(actor.d), actor.v);
      actor.x += velocity.x + actor.vx;
      actor.y += velocity.y + actor.vy;
      actor.d += actor.vd;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::WaveX:
  case EclOpcode::WaveY: {
    const auto &args = Args<EclWaveArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      actor.amp = args.amplitude;
      actor.vd = args.angle_delta;
      if (instruction.Opcode() == EclOpcode::WaveX) {
        actor.vx = absolute_velocity_x(args.velocity);
        actor.vy = actor.y;
      } else {
        actor.vy = args.velocity;
        actor.vx = actor.x;
      }
    }
    if (--actor.script.wait_counter != 0) {
      if (instruction.Opcode() == EclOpcode::WaveX) {
        actor.x += actor.vx;
        actor.y =
            actor.vy + math::RoundedPolarVector(math::AngleFromLegacy(actor.d),
                                                PixelToWorld(actor.amp))
                           .y;
      } else {
        actor.y += actor.vy;
        actor.x =
            actor.vx + math::RoundedPolarVector(math::AngleFromLegacy(actor.d),
                                                PixelToWorld(actor.amp))
                           .y;
      }
      actor.d += actor.vd;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::MoveX:
  case EclOpcode::MoveY: {
    const auto &args = Args<EclAxisMoveArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      if (instruction.Opcode() == EclOpcode::MoveX) {
        actor.vx = (PixelToWorld(args.coordinate) - actor.x) /
                   actor.script.wait_counter;
        actor.vy = 0;
      } else {
        actor.vy = (PixelToWorld(args.coordinate) - actor.y) /
                   actor.script.wait_counter;
        actor.vx = 0;
      }
    }
    if (--actor.script.wait_counter != 0) {
      if (instruction.Opcode() == EclOpcode::MoveX) {
        actor.x += actor.vx;
      } else {
        actor.y += actor.vy;
      }
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::MoveXY: {
    const auto &args = Args<EclPointMoveArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      actor.vx = (PixelToWorld(args.x) - actor.x) / actor.script.wait_counter;
      actor.vy = (PixelToWorld(args.y) - actor.y) / actor.script.wait_counter;
    }
    if (--actor.script.wait_counter != 0) {
      actor.x += actor.vx;
      actor.y += actor.vy;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::MoveToPlayerX:
  case EclOpcode::MoveToPlayerY:
  case EclOpcode::MoveToPlayer: {
    const auto frames = Args<EclDurationArguments>(instruction).frames;
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = frames + 1;
      actor.vx =
          instruction.Opcode() == EclOpcode::MoveToPlayerY
              ? 0
              : (host_.GetPlayer().X() - actor.x) / actor.script.wait_counter;
      actor.vy =
          instruction.Opcode() == EclOpcode::MoveToPlayerX
              ? 0
              : (host_.GetPlayer().Y() - actor.y) / actor.script.wait_counter;
    }
    if (--actor.script.wait_counter != 0) {
      actor.x += actor.vx;
      actor.y += actor.vy;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::Accelerate: {
    const auto &args = Args<EclAccelerationArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
    }
    if (--actor.script.wait_counter != 0) {
      actor.v += args.acceleration;
      const auto velocity =
          math::RoundedPolarVector(math::AngleFromLegacy(actor.d), actor.v);
      actor.x += velocity.x;
      actor.y += velocity.y;
      return Step::Repeat;
    }
    return Step::Advance;
  }

  case EclOpcode::AccelerateTo:
    return Step::Yield;

  case EclOpcode::GravityBounce: {
    const auto gravity = Args<EclSignedByteArguments>(instruction).value;
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = 9999;
      const auto velocity =
          math::RoundedPolarVector(math::AngleFromLegacy(actor.d), actor.v);
      actor.vx = velocity.x;
      actor.vy = velocity.y;
      actor.vd = gravity;
      actor.SetFlag(EnemyActorFlags::KeepOutsidePlayfield, true);
    } else {
      actor.x += actor.vx;
      actor.y += actor.vy;
      actor.vy += actor.vd;
      if (actor.x < playfield::kWorldLeft || actor.x > playfield::kWorldRight) {
        actor.vx = -actor.vx;
        actor.x += actor.vx;
      }
      if (actor.y < playfield::kWorldTop) {
        actor.vy = -actor.vy;
        actor.y += actor.vy;
      }
      if (actor.y > playfield::kWorldBottom + actor.hitbox_half_height) {
        actor.state = EnemyActorState::PendingRemoval;
      }
    }
    return Step::Repeat;
  }

  case EclOpcode::SetAngle:
    actor.d = absolute_angle(Args<EclByteArguments>(instruction).value);
    return Step::Advance;
  case EclOpcode::AddAngle:
    actor.d += relative_angle(Args<EclSignedByteArguments>(instruction).value);
    return Step::Advance;
  case EclOpcode::RandomAngle:
    actor.d = math::RandomInt() & 0xff;
    return Step::Advance;
  case EclOpcode::AimAtPlayer:
    actor.d = math::AngleToLegacy(
        math::AngleTo(static_cast<float>(host_.GetPlayer().X() - actor.x),
                      static_cast<float>(host_.GetPlayer().Y() - actor.y)));
    return Step::Advance;
  case EclOpcode::SetSpeed:
    actor.v = Args<EclSignedDwordArguments>(instruction).value;
    return Step::Advance;
  case EclOpcode::AddSpeed:
    actor.v += Args<EclSignedDwordArguments>(instruction).value;
    return Step::Advance;
  case EclOpcode::SetPosition: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.x = PixelToWorld(args.x);
    actor.y = PixelToWorld(args.y);
    return Step::Advance;
  }
  case EclOpcode::AddPosition: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.x += PixelToWorld(args.x);
    actor.y += PixelToWorld(args.y);
    return Step::Advance;
  }
  case EclOpcode::RandomAngleUp:
    actor.d = 128 + (math::RandomInt() & 0x7f);
    return Step::Advance;
  case EclOpcode::RandomAngleDown:
    actor.d = math::RandomInt() & 0x7f;
    return Step::Advance;
  case EclOpcode::SetSequenceAngle:
    actor.d = sequence_angle_;
    sequence_angle_ += sequence_angle_delta_;
    return Step::Advance;
  case EclOpcode::MoveToPlayerPosition:
    actor.x = host_.GetPlayer().X();
    actor.y = host_.GetPlayer().Y();
    return Step::Advance;

  case EclOpcode::RandomBoundedAngle: {
    const PixelLtrb bounds = {
        playfield::kWorldLeft + 150_px,
        playfield::kWorldTop +
            ((playfield::kWorldCenterY - playfield::kWorldTop - 40_px) / 3),
        playfield::kWorldRight - 150_px,
        playfield::kWorldCenterY -
            ((playfield::kWorldCenterY - playfield::kWorldTop - 40_px) / 3) -
            40_px};
    uint16_t base = 0;
    constexpr uint16_t range = 32;
    if (actor.y < bounds.top) {
      if (actor.x < bounds.left) {
        base = 16;
      } else if (actor.x > bounds.right) {
        base = 80;
      } else {
        base = 16 + 64 * ((math::RandomInt() >> 1) & 1);
      }
    } else if (actor.y > bounds.bottom) {
      if (actor.x < bounds.left) {
        base = static_cast<uint16_t>(-48);
      } else if (actor.x > bounds.right) {
        base = 144;
      } else {
        base = 176;
      }
    } else if (actor.x < bounds.left) {
      base = static_cast<uint16_t>(-16);
    } else if (actor.x > bounds.right) {
      base = 112;
    } else {
      base = ((math::RandomInt() >> 1) & 1) != 0 ? static_cast<uint16_t>(-16)
                                                 : 112;
    }
    actor.d = base + ((math::RandomInt() >> 1) % range);
    return Step::Advance;
  }

  case EclOpcode::RandomPosition:
    if (actor.x > playfield::kWorldCenterX) {
      actor.x =
          PixelToWorld(playfield::kCenterX) -
          ((math::RandomInt() % (playfield::kRight - playfield::kLeft - 100)) *
           32);
    } else {
      actor.x =
          PixelToWorld(playfield::kCenterX) +
          ((math::RandomInt() % (playfield::kRight - playfield::kLeft - 100)) *
           32);
    }
    actor.y = PixelToWorld(math::RandomInt() %
                           (playfield::kCenterY - playfield::kTop - 160)) +
              PixelToWorld(playfield::kTop + 40);
    return Step::Advance;

  case EclOpcode::MovePolar: {
    const auto length =
        PixelToWorld(Args<EclSignedWordArguments>(instruction).value);
    const auto offset =
        math::RoundedPolarVector(math::AngleFromLegacy(actor.d), length);
    actor.x += offset.x;
    actor.y += offset.y;
    return Step::Advance;
  }

  default:
    return Step::Halt;
  }
}

EclVm::Step EclVm::ExecuteBulletInstruction(EnemyActor &actor,
                                            const EclInstruction &instruction) {
  const auto fire = [&](bool scale,
                        BulletSpawnType type = BulletSpawnType::Normal) {
    auto info = MakeBulletSpawnInfo(actor.bullet_command, actor.x, actor.y,
                                    scale, host_.Session(), type);
    host_.Bullets().SpawnBullet(info);
  };

  switch (instruction.Opcode()) {
  case EclOpcode::FireBullet:
    fire(true);
    break;
  case EclOpcode::FireBulletUnscaled:
    fire(false);
    break;
  case EclOpcode::FireBulletLine:
    fire(false, BulletSpawnType::Line);
    break;
  case EclOpcode::FireExtraBullet:
    fire(false, BulletSpawnType::Extra01);
    break;
  case EclOpcode::SetAutoFire:
    actor.auto_fire_interval = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletOffset: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.bullet_command.x = PixelToWorld(args.x);
    actor.bullet_command.y = PixelToWorld(args.y);
    break;
  }
  case EclOpcode::SetBulletCommand:
    actor.bullet_command.cmd = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletAngle: {
    const auto &args = Args<EclBytePairArguments>(instruction);
    actor.bullet_command.d = args.first;
    actor.bullet_command.dw = args.second;
    break;
  }
  case EclOpcode::AddBulletAngle: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    actor.bullet_command.d += args.first;
    actor.bullet_command.dw += args.second;
    break;
  }
  case EclOpcode::AimBulletAtPlayer:
    actor.bullet_command.d = math::AngleToLegacy(
        math::AngleTo(static_cast<float>(host_.GetPlayer().X() - actor.x),
                      static_cast<float>(host_.GetPlayer().Y() - actor.y)));
    break;
  case EclOpcode::SyncBulletAngle:
    actor.bullet_command.d = actor.d;
    break;
  case EclOpcode::SetBulletCount: {
    const auto &args = Args<EclBytePairArguments>(instruction);
    actor.bullet_command.n = args.first;
    actor.bullet_command.ns = args.second;
    break;
  }
  case EclOpcode::AddBulletCount: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    actor.bullet_command.n += args.first;
    actor.bullet_command.ns += args.second;
    break;
  }
  case EclOpcode::SetBulletSpeed: {
    const auto &args = Args<EclByteSignedByteArguments>(instruction);
    actor.bullet_command.v = args.first;
    actor.bullet_command.a = args.second;
    break;
  }
  case EclOpcode::AddBulletSpeed: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    const auto previous = actor.bullet_command.v;
    actor.bullet_command.v = ((previous & 0x3f) + args.first) & 0x3f;
    actor.bullet_command.v |= previous & 0xc0;
    actor.bullet_command.a += args.second;
    break;
  }
  case EclOpcode::SetBulletOption:
    actor.bullet_command.option = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletType:
    actor.bullet_command.type = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletColor:
    actor.bullet_command.c = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletAngularVelocity:
    actor.bullet_command.vd = Args<EclSignedByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletRepeat:
    actor.bullet_command.rep = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::ClearBullets:
    host_.ClearBossProjectiles();
    host_.Bullets().Clear();
    host_.ClearRegular();
    break;
  case EclOpcode::BulletsToItems:
    host_.Bullets().ConvertBulletsToItems(
        Args<EclByteArguments>(instruction).value);
    break;
  default:
    return Step::Halt;
  }
  return Step::Advance;
}

EclVm::Step EclVm::ExecuteLaserInstruction(EnemyActor &actor,
                                           const EclInstruction &instruction) {
  const auto fire_reflect = [&](bool unscaled) {
    host_.Bullets().SpawnReflect(ReflectSpawnInfo{
        .no_scaling = unscaled,
        .x = actor.x + actor.laser_command.x,
        .y = actor.y + actor.laser_command.y,
        .v = static_cast<float>(actor.laser_command.v),
        .w = actor.laser_command.w,
        .l = actor.laser_command.l,
        .l2 = actor.laser_command.l2,
        .angle = math::AngleFromLegacy(actor.laser_command.d),
        .dw = actor.laser_command.dw,
        .n = actor.laser_command.n,
        .c = actor.laser_command.c,
        .aimed = (actor.laser_command.cmd & 0x08) != 0,
        .pattern = bullet_common::DecodePattern(actor.laser_command.cmd),
        .type = actor.laser_command.type == 1 ? ReflectLaserType::Reflect
                                              : ReflectLaserType::Short,
    });
  };

  switch (instruction.Opcode()) {
  case EclOpcode::FireLaser:
    fire_reflect(false);
    break;
  case EclOpcode::FireLaserUnscaled:
    fire_reflect(true);
    break;
  case EclOpcode::SetLaserCommand:
    actor.laser_command.cmd = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserLength:
    actor.laser_command.l = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::AddLaserLength:
    actor.laser_command.l += Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserStartLength:
    actor.laser_command.l2 = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserAngle: {
    const auto &args = Args<EclBytePairArguments>(instruction);
    actor.laser_command.d = args.first;
    actor.laser_command.dw = args.second;
    break;
  }
  case EclOpcode::AddLaserAngle: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    actor.laser_command.d += args.first;
    actor.laser_command.dw += args.second;
    break;
  }
  case EclOpcode::AimLaserAtPlayer:
    actor.laser_command.d = math::AngleToLegacy(
        math::AngleTo(static_cast<float>(host_.GetPlayer().X() - actor.x),
                      static_cast<float>(host_.GetPlayer().Y() - actor.y)));
    break;
  case EclOpcode::SyncLaserAngle:
    actor.laser_command.d = actor.d;
    break;
  case EclOpcode::SetLaserCount:
    actor.laser_command.n = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::AddLaserCount:
    actor.laser_command.n += Args<EclSignedByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserSpeed:
  case EclOpcode::AddLaserSpeed:
    // The original runtime assigned in both cases; preserve that format quirk.
    actor.laser_command.v = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserColor:
    actor.laser_command.c = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserType:
    actor.laser_command.type = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserWidth:
    actor.laser_command.w = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserOffset: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.laser_command.x = PixelToWorld(args.x);
    actor.laser_command.y = PixelToWorld(args.y);
    break;
  }
  case EclOpcode::SpawnLongLaser:
    if (host_.Bullets().SpawnLongLaser(LongLaserSpawnInfo{
            .enemy = &actor,
            .enemy_id = actor.long_laser_count,
            .dx = actor.laser_command.x,
            .dy = actor.laser_command.y,
            .v = actor.laser_command.v,
            .w = actor.laser_command.w,
            .angle = math::AngleFromLegacy(actor.laser_command.d),
            .c = actor.laser_command.c,
            .type = static_cast<LongLaserType>(actor.laser_command.type),
        })) {
      ++actor.long_laser_count;
    }
    break;
  case EclOpcode::OpenLongLaser:
    host_.Bullets().ControlLongLaser(
        &actor, Args<EclLongLaserArguments>(instruction).id,
        LongLaserUpdateInfo{LongLaserUpdateInfo::Command::Open});
    break;
  case EclOpcode::CloseLongLaser: {
    const auto id = Args<EclLongLaserArguments>(instruction).id;
    host_.Bullets().ControlLongLaser(
        &actor, id, LongLaserUpdateInfo{LongLaserUpdateInfo::Command::Close});
    if (id == kEclAllLongLasers) {
      actor.long_laser_count = 0;
    } else {
      --actor.long_laser_count;
    }
    break;
  }
  case EclOpcode::CloseLongLaserToLine:
    host_.Bullets().ControlLongLaser(
        &actor, Args<EclLongLaserArguments>(instruction).id,
        LongLaserUpdateInfo{LongLaserUpdateInfo::Command::CloseToLine});
    break;
  case EclOpcode::AddLongLaserAngle: {
    const auto &args = Args<EclLongLaserArguments>(instruction);
    host_.Bullets().ControlLongLaser(
        &actor, args.id,
        LongLaserUpdateInfo{LongLaserUpdateInfo::Command::AdjustAngle, 0.0f,
                            static_cast<float>(args.angle_delta) *
                                math::kLegacyAngleStep});
    break;
  }
  case EclOpcode::FireHomingLaser:
    host_.Bullets().SpawnHoming(HomingSpawnInfo{
        .x = actor.x + actor.laser_command.x,
        .y = actor.y + actor.laser_command.y,
        .angle = math::AngleFromLegacy(actor.laser_command.d),
        .dw = actor.laser_command.dw,
        .n = actor.laser_command.n,
        .c = actor.laser_command.c,
        .type = actor.laser_command.type == 1 ? HomingType::Type1
                                              : HomingType::None,
    });
    return Step::Yield;
  default:
    return Step::Halt;
  }
  return Step::Advance;
}

EclVm::Step EclVm::ExecuteActorInstruction(EnemyActor &actor,
                                           const EclInstruction &instruction) {
  switch (instruction.Opcode()) {
  case EclOpcode::EnableDraw:
    actor.SetFlag(EnemyActorFlags::Draw, true);
    break;
  case EclOpcode::DisableDraw:
    actor.SetFlag(EnemyActorFlags::Draw, false);
    break;
  case EclOpcode::EnableClip:
    actor.SetFlag(EnemyActorFlags::KeepOutsidePlayfield, true);
    break;
  case EclOpcode::DisableClip:
    actor.SetFlag(EnemyActorFlags::KeepOutsidePlayfield, false);
    break;
  case EclOpcode::EnableDamage:
    actor.SetFlag(EnemyActorFlags::Damageable, true);
    break;
  case EclOpcode::DisableDamage:
    actor.SetFlag(EnemyActorFlags::Damageable, false);
    break;
  case EclOpcode::EnablePlayerCollision:
    actor.SetFlag(EnemyActorFlags::CollidesWithPlayer, true);
    break;
  case EclOpcode::DisablePlayerCollision:
    actor.SetFlag(EnemyActorFlags::CollidesWithPlayer, false);
    break;
  case EclOpcode::EnableHorizontalMirror:
    if (actor.x < playfield::kWorldCenterX) {
      actor.SetFlag(EnemyActorFlags::HorizontalMirror, true);
    } else {
      actor.SetFlag(EnemyActorFlags::HorizontalMirror, false);
    }
    break;
  case EclOpcode::DisableHorizontalMirror:
    actor.SetFlag(EnemyActorFlags::HorizontalMirror, false);
    break;
  case EclOpcode::SetAnimation: {
    const auto &args = Args<EclAnimationArguments>(instruction);
    if (args.pattern >= host_.Animations().size() ||
        host_.Animations()[args.pattern].n == 0) {
      actor.state = EnemyActorState::PendingRemoval;
      return Step::Halt;
    }
    actor.animation = actor.damage_animation = args.pattern;
    actor.animation_speed = args.speed;
    actor.hitbox_half_height = host_.Animations()[actor.animation].size.h << 5;
    actor.hitbox_half_width = host_.Animations()[actor.animation].size.w << 5;
    actor.animation_frame = 0;
    break;
  }
  case EclOpcode::PlaySound:
    audio_.PlaySfx(
        static_cast<SfxId>(Args<EclByteArguments>(instruction).value), actor.x);
    break;
  case EclOpcode::BossAction:
    host_.HandleBossAction(actor,
                           Args<EclBossActionArguments>(instruction).action);
    break;
  case EclOpcode::SetSequenceAngleDelta:
    sequence_angle_delta_ = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SpawnEnemy:
  case EclOpcode::SpawnEnemyWithAngle: {
    const auto &args = Args<EclSpawnEnemyArguments>(instruction);
    WorldPoint position{&actor.x, &actor.y};
    position.x += PixelToWorld(args.offset_x);
    position.y += PixelToWorld(args.offset_y);
    auto *spawned = host_.SpawnRegular(position, args.script_id);
    if (spawned != nullptr && args.angle_source) {
      spawned->d = ReadValue(actor, *args.angle_source);
    }
    break;
  }
  case EclOpcode::SetHitbox: {
    const auto &args = Args<EclHitboxArguments>(instruction);
    actor.hitbox_half_width = PixelToWorld(args.width);
    actor.hitbox_half_height = PixelToWorld(args.height);
    break;
  }
  case EclOpcode::SetItem: {
    const auto value = Args<EclByteArguments>(instruction).value;
    actor.item = value <= std::to_underlying(ItemKind::Bomb)
                     ? static_cast<ItemKind>(value)
                     : ItemKind::None;
    return Step::Yield;
  }
  case EclOpcode::Stage4Effect: {
    host_.Stage().CommandRocks(
        Args<EclStage4EffectArguments>(instruction).command);
    break;
  }
  case EclOpcode::SetDamageAnimation:
    if (const auto pattern = Args<EclByteArguments>(instruction).value;
        pattern < host_.Animations().size() &&
        host_.Animations()[pattern].n != 0) {
      actor.damage_animation = pattern;
    } else {
      actor.state = EnemyActorState::PendingRemoval;
      return Step::Halt;
    }
    break;
  case EclOpcode::BitLaser:
    host_.ControlBitLaser(actor,
                          Args<EclBitLaserArguments>(instruction).command);
    break;
  case EclOpcode::BitAttack:
    host_.SetBitAttack(actor, Args<EclScriptArguments>(instruction).script_id);
    break;
  case EclOpcode::BitCommand: {
    const auto &args = Args<EclCommandValueArguments>(instruction);
    host_.ControlBits(actor, args.command, args.value);
    break;
  }
  case EclOpcode::SpawnBoss: {
    const WorldPoint position{&actor.x, &actor.y};
    host_.SpawnBoss(position, Args<EclScriptArguments>(instruction).script_id);
    break;
  }
  case EclOpcode::SpawnCircleEffect: {
    const auto &args = Args<EclCircleEffectArguments>(instruction);
    effects_.SpawnCircle(actor.x + PixelToWorld(args.offset_x),
                         actor.y + PixelToWorld(args.offset_y), args.effect);
    break;
  }
  case EclOpcode::Stage3Effect:
    host_.Stage().Command(stage::BackgroundCommand::Stage3Stars, effects_);
    return Step::Yield;
  default:
    return Step::Halt;
  }
  return Step::Advance;
}

EclVm::Step EclVm::ExecuteRegisterInstruction(EnemyActor &actor,
                                              const EclInstruction &instruction,
                                              int &comparison) {
  switch (instruction.Opcode()) {
  case EclOpcode::MoveValue: {
    const auto &args = Args<EclMoveValueArguments>(instruction);
    WriteValue(actor, args.destination, ReadValue(actor, args.source));
    break;
  }
  case EclOpcode::SetRegister: {
    const auto &args = Args<EclRegisterConstantArguments>(instruction);
    actor.script.registers[RegisterIndex(args.destination)] = args.value;
    break;
  }
  case EclOpcode::AddValue:
  case EclOpcode::SubtractValue: {
    const auto &args = Args<EclRegisterValueArguments>(instruction);
    auto &destination = actor.script.registers[RegisterIndex(args.destination)];
    if (instruction.Opcode() == EclOpcode::AddValue) {
      destination += ReadValue(actor, args.source);
    } else {
      destination -= ReadValue(actor, args.source);
    }
    break;
  }
  case EclOpcode::Sine:
  case EclOpcode::Cosine: {
    const auto &args = Args<EclRegisterPairArguments>(instruction);
    auto &length = actor.script.registers[RegisterIndex(args.first)];
    const auto angle = static_cast<uint8_t>(
        actor.script.registers[RegisterIndex(args.second)]);
    const auto vector = math::RoundedPolarVector(math::AngleFromLegacy(angle),
                                                 static_cast<float>(length));
    length = instruction.Opcode() == EclOpcode::Sine ? vector.y : vector.x;
    break;
  }
  case EclOpcode::Modulo: {
    const auto &args = Args<EclRegisterConstantArguments>(instruction);
    if (args.value != 0) {
      actor.script.registers[RegisterIndex(args.destination)] %= args.value;
    }
    break;
  }
  case EclOpcode::Random: {
    const auto destination =
        Args<EclRegisterArguments>(instruction).destination;
    actor.script.registers[RegisterIndex(destination)] =
        static_cast<uint32_t>(math::RandomInt()) *
        static_cast<uint32_t>(math::RandomInt());
    break;
  }
  case EclOpcode::CompareRegisters: {
    const auto &args = Args<EclRegisterPairArguments>(instruction);
    comparison = std::bit_cast<int32_t>(ReadValue(actor, args.first) -
                                        ReadValue(actor, args.second));
    break;
  }
  case EclOpcode::CompareConstant: {
    const auto &args = Args<EclRegisterSignedConstantArguments>(instruction);
    comparison = std::bit_cast<int32_t>(ReadValue(actor, args.destination) -
                                        static_cast<uint32_t>(args.value));
    break;
  }
  case EclOpcode::JumpGreater:
  case EclOpcode::JumpLess:
  case EclOpcode::JumpEqual: {
    const bool take =
        (instruction.Opcode() == EclOpcode::JumpGreater && comparison > 0) ||
        (instruction.Opcode() == EclOpcode::JumpLess && comparison < 0) ||
        (instruction.Opcode() == EclOpcode::JumpEqual && comparison == 0);
    if (take) {
      actor.script.position = Args<EclJumpArguments>(instruction).target;
      return Step::Jump;
    }
    break;
  }
  case EclOpcode::Increment:
  case EclOpcode::Decrement: {
    const auto destination =
        Args<EclRegisterArguments>(instruction).destination;
    auto &value = actor.script.registers[RegisterIndex(destination)];
    instruction.Opcode() == EclOpcode::Increment ? ++value : --value;
    break;
  }
  default:
    return Step::Halt;
  }
  return Step::Advance;
}

void EclVm::CheckInterrupts(EnemyActor &actor) {
  const auto trigger = [&actor](const EclInterruptState &interrupt) {
    actor.script.position = *interrupt.target;
    actor.script.wait_counter = 0;
    actor.script.loop_counter = 0;
    actor.auto_fire_interval = 0;
  };

  for (size_t index = 0; index < actor.script.interrupts.size(); ++index) {
    const auto &interrupt = actor.script.interrupts[index];
    if (!interrupt.target) {
      continue;
    }

    const auto kind = static_cast<EclInterrupt>(index);
    bool should_trigger = false;
    switch (kind) {
    case EclInterrupt::BossCount:
      should_trigger = host_.BossCount() <= interrupt.threshold;
      break;
    case EclInterrupt::Hp:
      should_trigger = actor.hp <= interrupt.threshold;
      break;
    case EclInterrupt::Timer:
      should_trigger = actor.script.interrupt_timer > interrupt.threshold;
      if (!should_trigger) {
        ++actor.script.interrupt_timer;
      }
      break;
    case EclInterrupt::BitCount:
      should_trigger = host_.BitCount(actor) <= interrupt.threshold;
      break;
    }

    if (should_trigger) {
      trigger(interrupt);
      if (kind == EclInterrupt::Timer) {
        actor.script.interrupt_timer = 0;
      }
      return;
    }
  }
}

uint32_t EclVm::ReadValue(const EnemyActor &actor, EclValue value) {
  if (IsEclRegister(value)) {
    return actor.script.registers[RegisterIndex(value)];
  }
  switch (value) {
  case EclValue::LaserAngle:
    return actor.laser_command.d;
  case EclValue::LaserAngleDelta:
    return actor.laser_command.dw;
  case EclValue::LaserCount:
    return actor.laser_command.n;
  case EclValue::LaserColor:
    return actor.laser_command.c;
  case EclValue::LaserLength:
    return actor.laser_command.l;
  case EclValue::LaserSpeed:
    return actor.laser_command.v;
  case EclValue::BulletAngle:
    return actor.bullet_command.d;
  case EclValue::BulletAngleDelta:
    return actor.bullet_command.dw;
  case EclValue::BulletCount:
    return actor.bullet_command.n;
  case EclValue::BulletRapidCount:
    return actor.bullet_command.ns;
  case EclValue::BulletSpeed:
    return actor.bullet_command.v;
  case EclValue::BulletColor:
    return actor.bullet_command.c;
  case EclValue::BulletAcceleration:
    return actor.bullet_command.a;
  case EclValue::BulletRepeat:
    return actor.bullet_command.rep;
  case EclValue::BulletAngularVelocity:
    return actor.bullet_command.vd;
  case EclValue::ActorX:
    return actor.x;
  case EclValue::ActorY:
    return actor.y;
  case EclValue::ActorAngle:
    return actor.d;
  default:
    return 0;
  }
}

void EclVm::WriteValue(EnemyActor &actor, EclValue destination,
                       uint32_t value) {
  if (IsEclRegister(destination)) {
    actor.script.registers[RegisterIndex(destination)] = value;
    return;
  }
  switch (destination) {
  case EclValue::LaserAngle:
    actor.laser_command.d = value;
    break;
  case EclValue::LaserAngleDelta:
    actor.laser_command.dw = value;
    break;
  case EclValue::LaserCount:
    actor.laser_command.n = value;
    break;
  case EclValue::LaserColor:
    actor.laser_command.c = value;
    break;
  case EclValue::LaserLength:
    actor.laser_command.l = value;
    break;
  case EclValue::LaserSpeed:
    actor.laser_command.v = value;
    break;
  case EclValue::BulletAngle:
    actor.bullet_command.d = value;
    break;
  case EclValue::BulletAngleDelta:
    actor.bullet_command.dw = value;
    break;
  case EclValue::BulletCount:
    actor.bullet_command.n = value;
    break;
  case EclValue::BulletRapidCount:
    actor.bullet_command.ns = value;
    break;
  case EclValue::BulletSpeed:
    actor.bullet_command.v = value;
    break;
  case EclValue::BulletColor:
    actor.bullet_command.c = value;
    break;
  case EclValue::BulletAcceleration:
    actor.bullet_command.a = value;
    break;
  case EclValue::BulletRepeat:
    actor.bullet_command.rep = value;
    break;
  case EclValue::BulletAngularVelocity:
    actor.bullet_command.vd = value;
    break;
  case EclValue::ActorX:
    actor.x = value;
    break;
  case EclValue::ActorY:
    actor.y = value;
    break;
  case EclValue::ActorAngle:
    actor.d = value;
    break;
  default:
    break;
  }
}
