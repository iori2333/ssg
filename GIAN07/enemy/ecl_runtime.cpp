///
/// ECL runtime - typed instruction dispatch and per-frame execution
///

#include <algorithm>
#include <bit>
#include <utility>
#include <variant>

#include "boss_manager.h"
#include "ecl_program.h"
#include "enemy.h"
#include "enemy_system.h"

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/game_manager.h"
#include "core/gian.h"
#include "core/level.h"
#include "effect/effect_manager.h"
#include "player/player.h"
#include "stage/stage_session.h"
#include "util/cast.h"
#include "util/ut_math.h"

namespace {

template <typename Arguments>
const Arguments &Args(const EclInstruction &instruction) {
  return std::get<Arguments>(instruction.arguments);
}

size_t RegisterIndex(EclValue value) { return static_cast<size_t>(value); }

} // namespace

void EnemySystem::Execute(EnemyActor *actor) {
  int comparison = 0;
  while (const auto *instruction =
             program_.InstructionAt(actor->script.position)) {
    switch (ExecuteInstruction(*actor, *instruction, comparison)) {
    case EclStep::Advance:
      ++actor->script.position;
      break;
    case EclStep::Jump:
      break;
    case EclStep::Yield:
      ++actor->script.position;
      return;
    case EclStep::Repeat:
    case EclStep::Halt:
      return;
    }
  }

  actor->flag = EF_DELETE;
}

EnemySystem::EclStep EnemySystem::ExecuteInstruction(
    EnemyActor &actor, const EclInstruction &instruction, int &comparison) {
  const auto opcode = std::to_underlying(instruction.opcode);
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
  return EclStep::Halt;
}

EnemySystem::EclStep
EnemySystem::ExecuteControlInstruction(EnemyActor &actor,
                                       const EclInstruction &instruction) {
  switch (instruction.opcode) {
  case EclOpcode::Setup: {
    const auto &args = Args<EclSetupArguments>(instruction);
    actor.hp = args.hp;
    actor.score = args.score;
    if (actor.hp == 0) {
      bosses_.KillActors();
    }
    return EclStep::Advance;
  }
  case EclOpcode::End:
    if (actor.LLaserRef != 0U) {
      bullets_->ControlLongLaser(
          &actor, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
    }
    actor.flag = EF_DELETE;
    return EclStep::Halt;

  case EclOpcode::Jump:
    actor.script.position = Args<EclJumpArguments>(instruction).target;
    return EclStep::Jump;

  case EclOpcode::Loop: {
    const auto &args = Args<EclLoopArguments>(instruction);
    if (actor.script.loop_counter == 0) {
      actor.script.loop_counter = args.count + 1;
    }
    if (--actor.script.loop_counter != 0) {
      actor.script.position = args.target;
      return EclStep::Jump;
    }
    return EclStep::Advance;
  }

  case EclOpcode::Call:
    actor.script.return_position = actor.script.position + 1;
    actor.script.position = Args<EclJumpArguments>(instruction).target;
    return EclStep::Jump;

  case EclOpcode::Return:
    actor.script.position = actor.script.return_position;
    return EclStep::Jump;

  case EclOpcode::JumpHpGreater:
  case EclOpcode::JumpHpLess:
  case EclOpcode::JumpFrameGreater:
  case EclOpcode::JumpFrameLess: {
    const auto &args = Args<EclConditionalJumpArguments>(instruction);
    bool take_branch = false;
    switch (instruction.opcode) {
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
      return EclStep::Jump;
    }
    return EclStep::Advance;
  }

  case EclOpcode::JumpDifficulty: {
    const auto &targets = Args<EclDifficultyJumpArguments>(instruction).targets;
    switch (game_->EffectiveLevel()) {
    case GameLevel::EASY:
      actor.script.position = targets[0];
      break;
    default:
    case GameLevel::NORMAL:
      actor.script.position = targets[1];
      break;
    case GameLevel::HARD:
      actor.script.position = targets[2];
      break;
    case GameLevel::LUNATIC:
      actor.script.position = targets[3];
      break;
    }
    return EclStep::Jump;
  }

  case EclOpcode::JumpDirection: {
    const auto difference = static_cast<uint8_t>(
        abs(atan8(player_->X() - actor.x, player_->Y() - actor.y) - actor.d));
    if (difference < 4) {
      actor.script.position = Args<EclJumpArguments>(instruction).target;
      return EclStep::Jump;
    }
    return EclStep::Advance;
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
    return EclStep::Advance;
  }

  case EclOpcode::ClearInterrupt: {
    const auto interrupt = Args<EclInterruptArguments>(instruction).interrupt;
    actor.script.interrupts[static_cast<size_t>(interrupt)].target.reset();
    return EclStep::Advance;
  }

  default:
    return EclStep::Halt;
  }
}

EnemySystem::EclStep
EnemySystem::ExecuteMovementInstruction(EnemyActor &actor,
                                        const EclInstruction &instruction) {
  const auto absolute_angle = [&actor](uint8_t angle) -> uint8_t {
    return (actor.flag & EF_RLCHG) ? (128 - angle) : angle;
  };
  const auto absolute_velocity_x = [&actor](int velocity) {
    return (actor.flag & EF_RLCHG) ? -velocity : velocity;
  };
  const auto relative_angle = [&actor](int8_t angle) -> int8_t {
    return (actor.flag & EF_RLCHG) ? static_cast<int8_t>(-angle) : angle;
  };
  const auto continue_duration = [&actor](uint16_t frames) {
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = frames + 1;
    }
    return --actor.script.wait_counter != 0;
  };

  switch (instruction.opcode) {
  case EclOpcode::Wait:
  case EclOpcode::WaitScroll:
    return continue_duration(Args<EclDurationArguments>(instruction).frames)
               ? EclStep::Repeat
               : EclStep::Advance;

  case EclOpcode::Move: {
    const auto frames = Args<EclDurationArguments>(instruction).frames;
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = frames + 1;
      actor.vx = cosl(actor.d, actor.v);
      actor.vy = sinl(actor.d, actor.v);
    }
    if (--actor.script.wait_counter != 0) {
      actor.x += actor.vx;
      actor.y += actor.vy;
      return EclStep::Repeat;
    }
    return EclStep::Advance;
  }

  case EclOpcode::Rotate: {
    const auto &args = Args<EclRotateArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      actor.vd = relative_angle(args.angle_delta);
    }
    if (--actor.script.wait_counter != 0) {
      actor.x += cosl(actor.d, actor.v);
      actor.y += sinl(actor.d, actor.v);
      actor.d += actor.vd;
      return EclStep::Repeat;
    }
    return EclStep::Advance;
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
      actor.x += cosl(actor.d, actor.v) + actor.vx;
      actor.y += sinl(actor.d, actor.v) + actor.vy;
      actor.d += actor.vd;
      return EclStep::Repeat;
    }
    return EclStep::Advance;
  }

  case EclOpcode::WaveX:
  case EclOpcode::WaveY: {
    const auto &args = Args<EclWaveArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      actor.amp = args.amplitude;
      actor.vd = args.angle_delta;
      if (instruction.opcode == EclOpcode::WaveX) {
        actor.vx = absolute_velocity_x(args.velocity);
        actor.vy = actor.y;
      } else {
        actor.vy = args.velocity;
        actor.vx = actor.x;
      }
    }
    if (--actor.script.wait_counter != 0) {
      if (instruction.opcode == EclOpcode::WaveX) {
        actor.x += actor.vx;
        actor.y = actor.vy + sinl(actor.d, actor.amp << 6);
      } else {
        actor.y += actor.vy;
        actor.x = actor.vx + sinl(actor.d, actor.amp << 6);
      }
      actor.d += actor.vd;
      return EclStep::Repeat;
    }
    return EclStep::Advance;
  }

  case EclOpcode::MoveX:
  case EclOpcode::MoveY: {
    const auto &args = Args<EclAxisMoveArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
      if (instruction.opcode == EclOpcode::MoveX) {
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
      if (instruction.opcode == EclOpcode::MoveX) {
        actor.x += actor.vx;
      } else {
        actor.y += actor.vy;
      }
      return EclStep::Repeat;
    }
    return EclStep::Advance;
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
      return EclStep::Repeat;
    }
    return EclStep::Advance;
  }

  case EclOpcode::MoveToPlayerX:
  case EclOpcode::MoveToPlayerY:
  case EclOpcode::MoveToPlayer: {
    const auto frames = Args<EclDurationArguments>(instruction).frames;
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = frames + 1;
      actor.vx = instruction.opcode == EclOpcode::MoveToPlayerY
                     ? 0
                     : (player_->X() - actor.x) / actor.script.wait_counter;
      actor.vy = instruction.opcode == EclOpcode::MoveToPlayerX
                     ? 0
                     : (player_->Y() - actor.y) / actor.script.wait_counter;
    }
    if (--actor.script.wait_counter != 0) {
      actor.x += actor.vx;
      actor.y += actor.vy;
      return EclStep::Repeat;
    }
    return EclStep::Advance;
  }

  case EclOpcode::Accelerate: {
    const auto &args = Args<EclAccelerationArguments>(instruction);
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = args.frames + 1;
    }
    if (--actor.script.wait_counter != 0) {
      actor.v += args.acceleration;
      actor.x += cosl(actor.d, actor.v);
      actor.y += sinl(actor.d, actor.v);
      return EclStep::Repeat;
    }
    return EclStep::Advance;
  }

  case EclOpcode::AccelerateTo:
    return EclStep::Yield;

  case EclOpcode::GravityBounce: {
    const auto gravity = Args<EclSignedByteArguments>(instruction).value;
    if (actor.script.wait_counter == 0) {
      actor.script.wait_counter = 9999;
      actor.vx = cosl(actor.d, actor.v);
      actor.vy = sinl(actor.d, actor.v);
      actor.vd = gravity;
      actor.flag |= EF_CLIP;
    } else {
      actor.x += actor.vx;
      actor.y += actor.vy;
      actor.vy += actor.vd;
      if (actor.x < GX_MIN || actor.x > GX_MAX) {
        actor.vx = -actor.vx;
        actor.x += actor.vx;
      }
      if (actor.y < GY_MIN) {
        actor.vy = -actor.vy;
        actor.y += actor.vy;
      }
      if (actor.y > GY_MAX + actor.g_height) {
        actor.flag = EF_DELETE;
      }
    }
    return EclStep::Repeat;
  }

  case EclOpcode::SetAngle:
    actor.d = absolute_angle(Args<EclByteArguments>(instruction).value);
    return EclStep::Advance;
  case EclOpcode::AddAngle:
    actor.d += relative_angle(Args<EclSignedByteArguments>(instruction).value);
    return EclStep::Advance;
  case EclOpcode::RandomAngle:
    actor.d = rnd() & 0xff;
    return EclStep::Advance;
  case EclOpcode::AimAtPlayer:
    actor.d = atan8(player_->X() - actor.x, player_->Y() - actor.y);
    return EclStep::Advance;
  case EclOpcode::SetSpeed:
    actor.v = Args<EclSignedDwordArguments>(instruction).value;
    return EclStep::Advance;
  case EclOpcode::AddSpeed:
    actor.v += Args<EclSignedDwordArguments>(instruction).value;
    return EclStep::Advance;
  case EclOpcode::SetPosition: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.x = PixelToWorld(args.x);
    actor.y = PixelToWorld(args.y);
    return EclStep::Advance;
  }
  case EclOpcode::AddPosition: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.x += PixelToWorld(args.x);
    actor.y += PixelToWorld(args.y);
    return EclStep::Advance;
  }
  case EclOpcode::RandomAngleUp:
    actor.d = 128 + (rnd() & 0x7f);
    return EclStep::Advance;
  case EclOpcode::RandomAngleDown:
    actor.d = rnd() & 0x7f;
    return EclStep::Advance;
  case EclOpcode::SetSequenceAngle:
    actor.d = enemy_exdeg;
    enemy_exdeg += enemy_exdeg_d;
    return EclStep::Advance;
  case EclOpcode::MoveToPlayerPosition:
    actor.x = player_->X();
    actor.y = player_->Y();
    return EclStep::Advance;

  case EclOpcode::RandomBoundedAngle: {
    const PIXEL_LTRB bounds = {
        GX_MIN + (150 * 64), GY_MIN + ((GY_MID - GY_MIN - (40 * 64)) / 3),
        GX_MAX - (150 * 64),
        GY_MID - ((GY_MID - GY_MIN - (40 * 64)) / 3) - (40 * 64)};
    uint16_t base = 0;
    constexpr uint16_t range = 32;
    if (actor.y < bounds.top) {
      if (actor.x < bounds.left) {
        base = 16;
      } else if (actor.x > bounds.right) {
        base = 80;
      } else {
        base = 16 + 64 * ((rnd() >> 1) & 1);
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
      base = ((rnd() >> 1) & 1) != 0 ? static_cast<uint16_t>(-16) : 112;
    }
    actor.d = base + ((rnd() >> 1) % range);
    return EclStep::Advance;
  }

  case EclOpcode::RandomPosition:
    if (actor.x > GX_MID) {
      actor.x = (X_MID * 64) - ((rnd() % (X_MAX - X_MIN - 100)) * 32);
    } else {
      actor.x = (X_MID * 64) + ((rnd() % (X_MAX - X_MIN - 100)) * 32);
    }
    actor.y = ((rnd() % (Y_MID - Y_MIN - 160)) * 64) + ((Y_MIN + 40) * 64);
    return EclStep::Advance;

  case EclOpcode::MovePolar: {
    const auto length =
        PixelToWorld(Args<EclSignedWordArguments>(instruction).value);
    actor.x += cosl(actor.d, length);
    actor.y += sinl(actor.d, length);
    return EclStep::Advance;
  }

  default:
    return EclStep::Halt;
  }
}

EnemySystem::EclStep
EnemySystem::ExecuteBulletInstruction(EnemyActor &actor,
                                      const EclInstruction &instruction) {
  const auto fire = [&](bool scale,
                        BulletSpawnType type = BulletSpawnType::Normal) {
    auto info =
        MakeBulletSpawnInfo(actor.t_cmd, actor.x, actor.y, scale, *game_, type);
    bullets_->SpawnBullet(info);
  };

  switch (instruction.opcode) {
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
    actor.t_rep = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletOffset: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.t_cmd.x = PixelToWorld(args.x);
    actor.t_cmd.y = PixelToWorld(args.y);
    break;
  }
  case EclOpcode::SetBulletCommand:
    actor.t_cmd.cmd = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletAngle: {
    const auto &args = Args<EclBytePairArguments>(instruction);
    actor.t_cmd.d = args.first;
    actor.t_cmd.dw = args.second;
    break;
  }
  case EclOpcode::AddBulletAngle: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    actor.t_cmd.d += args.first;
    actor.t_cmd.dw += args.second;
    break;
  }
  case EclOpcode::AimBulletAtPlayer:
    actor.t_cmd.d = atan8(player_->X() - actor.x, player_->Y() - actor.y);
    break;
  case EclOpcode::SyncBulletAngle:
    actor.t_cmd.d = actor.d;
    break;
  case EclOpcode::SetBulletCount: {
    const auto &args = Args<EclBytePairArguments>(instruction);
    actor.t_cmd.n = args.first;
    actor.t_cmd.ns = args.second;
    break;
  }
  case EclOpcode::AddBulletCount: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    actor.t_cmd.n += args.first;
    actor.t_cmd.ns += args.second;
    break;
  }
  case EclOpcode::SetBulletSpeed: {
    const auto &args = Args<EclByteSignedByteArguments>(instruction);
    actor.t_cmd.v = args.first;
    actor.t_cmd.a = args.second;
    break;
  }
  case EclOpcode::AddBulletSpeed: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    const auto previous = actor.t_cmd.v;
    actor.t_cmd.v = ((previous & 0x3f) + args.first) & 0x3f;
    actor.t_cmd.v |= previous & 0xc0;
    actor.t_cmd.a += args.second;
    break;
  }
  case EclOpcode::SetBulletOption:
    actor.t_cmd.option = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletType:
    actor.t_cmd.type = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletColor:
    actor.t_cmd.c = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletAngularVelocity:
    actor.t_cmd.vd = Args<EclSignedByteArguments>(instruction).value;
    break;
  case EclOpcode::SetBulletRepeat:
    actor.t_cmd.rep = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::ClearBullets:
    bosses_.ClearProjectiles();
    bullets_->Clear();
    ClearRegular();
    break;
  case EclOpcode::BulletsToItems:
    bullets_->ToItems(Args<EclByteArguments>(instruction).value);
    break;
  default:
    return EclStep::Halt;
  }
  return EclStep::Advance;
}

EnemySystem::EclStep
EnemySystem::ExecuteLaserInstruction(EnemyActor &actor,
                                     const EclInstruction &instruction) {
  const auto fire_reflect = [&](bool unscaled) {
    bullets_->SpawnReflect(ReflectSpawnInfo{
        .no_scaling = unscaled,
        .x = actor.x + actor.l_cmd.x,
        .y = actor.y + actor.l_cmd.y,
        .v = actor.l_cmd.v,
        .w = actor.l_cmd.w,
        .l = actor.l_cmd.l,
        .l2 = actor.l_cmd.l2,
        .d = actor.l_cmd.d,
        .dw = actor.l_cmd.dw,
        .n = actor.l_cmd.n,
        .c = actor.l_cmd.c,
        .a = actor.l_cmd.a,
        .cmd = actor.l_cmd.cmd,
        .cmd_type = actor.l_cmd.type,
    });
  };

  switch (instruction.opcode) {
  case EclOpcode::FireLaser:
    fire_reflect(false);
    break;
  case EclOpcode::FireLaserUnscaled:
    fire_reflect(true);
    break;
  case EclOpcode::SetLaserCommand:
    actor.l_cmd.cmd = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserLength:
    actor.l_cmd.l = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::AddLaserLength:
    actor.l_cmd.l += Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserStartLength:
    actor.l_cmd.l2 = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserAngle: {
    const auto &args = Args<EclBytePairArguments>(instruction);
    actor.l_cmd.d = args.first;
    actor.l_cmd.dw = args.second;
    break;
  }
  case EclOpcode::AddLaserAngle: {
    const auto &args = Args<EclSignedBytePairArguments>(instruction);
    actor.l_cmd.d += args.first;
    actor.l_cmd.dw += args.second;
    break;
  }
  case EclOpcode::AimLaserAtPlayer:
    actor.l_cmd.d = atan8(player_->X() - actor.x, player_->Y() - actor.y);
    break;
  case EclOpcode::SyncLaserAngle:
    actor.l_cmd.d = actor.d;
    break;
  case EclOpcode::SetLaserCount:
    actor.l_cmd.n = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::AddLaserCount:
    actor.l_cmd.n += Args<EclSignedByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserSpeed:
  case EclOpcode::AddLaserSpeed:
    // The original runtime assigned in both cases; preserve that format quirk.
    actor.l_cmd.v = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserColor:
    actor.l_cmd.c = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserType:
    actor.l_cmd.type = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserWidth:
    actor.l_cmd.w = Args<EclSignedDwordArguments>(instruction).value;
    break;
  case EclOpcode::SetLaserOffset: {
    const auto &args = Args<EclPointArguments>(instruction);
    actor.l_cmd.x = PixelToWorld(args.x);
    actor.l_cmd.y = PixelToWorld(args.y);
    break;
  }
  case EclOpcode::SpawnLongLaser:
    if (bullets_->SpawnLongLaser(LongLaserSpawnInfo{
            .enemy = &actor,
            .enemy_id = actor.LLaserRef,
            .dx = actor.l_cmd.x,
            .dy = actor.l_cmd.y,
            .v = actor.l_cmd.v,
            .w = actor.l_cmd.w,
            .d = actor.l_cmd.d,
            .c = actor.l_cmd.c,
            .type = static_cast<LongLaserType>(actor.l_cmd.type),
        })) {
      ++actor.LLaserRef;
    }
    break;
  case EclOpcode::OpenLongLaser:
    bullets_->ControlLongLaser(
        &actor, Args<EclLongLaserArguments>(instruction).id,
        LongLaserUpdateInfo{LongLaserUpdateInfo::Command::Open});
    break;
  case EclOpcode::CloseLongLaser: {
    const auto id = Args<EclLongLaserArguments>(instruction).id;
    bullets_->ControlLongLaser(
        &actor, id, LongLaserUpdateInfo{LongLaserUpdateInfo::Command::Close});
    if (id == ECL_ALL_LONG_LASERS) {
      actor.LLaserRef = 0;
    } else {
      --actor.LLaserRef;
    }
    break;
  }
  case EclOpcode::CloseLongLaserToLine:
    bullets_->ControlLongLaser(
        &actor, Args<EclLongLaserArguments>(instruction).id,
        LongLaserUpdateInfo{LongLaserUpdateInfo::Command::CloseToLine});
    break;
  case EclOpcode::AddLongLaserAngle: {
    const auto &args = Args<EclLongLaserArguments>(instruction);
    bullets_->ControlLongLaser(
        &actor, args.id,
        LongLaserUpdateInfo{LongLaserUpdateInfo::Command::AdjustAngle, 0,
                            args.angle_delta});
    break;
  }
  case EclOpcode::FireHomingLaser:
    bullets_->SpawnHoming(HomingSpawnInfo{
        .x = actor.x + actor.l_cmd.x,
        .y = actor.y + actor.l_cmd.y,
        .d = actor.l_cmd.d,
        .dw = actor.l_cmd.dw,
        .n = actor.l_cmd.n,
        .c = actor.l_cmd.c,
        .type = actor.l_cmd.type,
    });
    return EclStep::Yield;
  default:
    return EclStep::Halt;
  }
  return EclStep::Advance;
}

EnemySystem::EclStep
EnemySystem::ExecuteActorInstruction(EnemyActor &actor,
                                     const EclInstruction &instruction) {
  switch (instruction.opcode) {
  case EclOpcode::EnableDraw:
    actor.flag |= EF_DRAW;
    break;
  case EclOpcode::DisableDraw:
    actor.flag &= ~EF_DRAW;
    break;
  case EclOpcode::EnableClip:
    actor.flag |= EF_CLIP;
    break;
  case EclOpcode::DisableClip:
    actor.flag &= ~EF_CLIP;
    break;
  case EclOpcode::EnableDamage:
    actor.flag |= EF_DAMAGE;
    break;
  case EclOpcode::DisableDamage:
    actor.flag &= ~EF_DAMAGE;
    break;
  case EclOpcode::EnablePlayerCollision:
    actor.flag |= EF_HITSB;
    break;
  case EclOpcode::DisablePlayerCollision:
    actor.flag &= ~EF_HITSB;
    break;
  case EclOpcode::EnableHorizontalMirror:
    if (actor.x < GX_MID) {
      actor.flag |= EF_RLCHG;
    } else {
      actor.flag &= ~EF_RLCHG;
    }
    break;
  case EclOpcode::DisableHorizontalMirror:
    actor.flag &= ~EF_RLCHG;
    break;
  case EclOpcode::SetAnimation: {
    const auto &args = Args<EclAnimationArguments>(instruction);
    actor.anm_ptn = actor.anm_ptnEx = args.pattern;
    actor.anm_sp = args.speed;
    actor.g_height = anime[actor.anm_ptn].size.h << 5;
    actor.g_width = anime[actor.anm_ptn].size.w << 5;
    actor.anm_c = 0;
    break;
  }
  case EclOpcode::PlaySound:
    Snd_SEPlay(static_cast<SfxId>(Args<EclByteArguments>(instruction).value),
               actor.x);
    break;
  case EclOpcode::BossAction:
    bosses_.HandleAction(&actor,
                         Args<EclBossActionArguments>(instruction).action);
    break;
  case EclOpcode::SetSequenceAngleDelta:
    enemy_exdeg_d = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::SpawnEnemy:
  case EclOpcode::SpawnEnemyWithAngle: {
    const auto &args = Args<EclSpawnEnemyArguments>(instruction);
    WORLD_POINT position{&actor.x, &actor.y};
    position.x += PixelToWorld(args.offset_x);
    position.y += PixelToWorld(args.offset_y);
    auto *spawned = SpawnRegular(position, args.script_id);
    if (spawned != nullptr && args.angle_source) {
      spawned->d = ReadValue(actor, *args.angle_source);
    }
    break;
  }
  case EclOpcode::SetHitbox: {
    const auto &args = Args<EclHitboxArguments>(instruction);
    actor.g_width = PixelToWorld(args.width);
    actor.g_height = PixelToWorld(args.height);
    break;
  }
  case EclOpcode::SetItem:
    actor.item = Args<EclByteArguments>(instruction).value;
    return EclStep::Yield;
  case EclOpcode::Stage4Effect: {
    const auto command = Args<EclByteArguments>(instruction).value;
    Effects.SendCmdStg4Rocks(command,
                             command == STG4ROCK_ACCMOVE2 ? actor.d : 0);
    break;
  }
  case EclOpcode::SetDamageAnimation:
    actor.anm_ptnEx = Args<EclByteArguments>(instruction).value;
    break;
  case EclOpcode::BitLaser:
    bosses_.ControlBitLaser(&actor,
                            Args<EclBitLaserArguments>(instruction).command);
    break;
  case EclOpcode::BitAttack:
    bosses_.SetBitAttack(&actor,
                         Args<EclScriptArguments>(instruction).script_id);
    break;
  case EclOpcode::BitCommand: {
    const auto &args = Args<EclCommandValueArguments>(instruction);
    bosses_.ControlBits(&actor, args.command, args.value);
    break;
  }
  case EclOpcode::SpawnBoss: {
    const WORLD_POINT position{&actor.x, &actor.y};
    bosses_.SpawnFromEcl(position,
                         Args<EclScriptArguments>(instruction).script_id);
    break;
  }
  case EclOpcode::SpawnCircleEffect: {
    const auto &args = Args<EclCircleEffectArguments>(instruction);
    Effects.SpawnCircleEffect(actor.x + PixelToWorld(args.offset_x),
                              actor.y + PixelToWorld(args.offset_y),
                              args.effect);
    break;
  }
  case EclOpcode::Stage3Effect:
    stage_->Command(stage::BackgroundCommand::Stage3Stars, Effects);
    return EclStep::Yield;
  default:
    return EclStep::Halt;
  }
  return EclStep::Advance;
}

EnemySystem::EclStep EnemySystem::ExecuteRegisterInstruction(
    EnemyActor &actor, const EclInstruction &instruction, int &comparison) {
  switch (instruction.opcode) {
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
    if (instruction.opcode == EclOpcode::AddValue) {
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
    const auto angle =
        Cast::down<uint8_t>(actor.script.registers[RegisterIndex(args.second)]);
    length = instruction.opcode == EclOpcode::Sine ? sinl(angle, length)
                                                   : cosl(angle, length);
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
        Cast::up<uint32_t>(rnd()) * rnd();
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
        (instruction.opcode == EclOpcode::JumpGreater && comparison > 0) ||
        (instruction.opcode == EclOpcode::JumpLess && comparison < 0) ||
        (instruction.opcode == EclOpcode::JumpEqual && comparison == 0);
    if (take) {
      actor.script.position = Args<EclJumpArguments>(instruction).target;
      return EclStep::Jump;
    }
    break;
  }
  case EclOpcode::Increment:
  case EclOpcode::Decrement: {
    const auto destination =
        Args<EclRegisterArguments>(instruction).destination;
    auto &value = actor.script.registers[RegisterIndex(destination)];
    instruction.opcode == EclOpcode::Increment ? ++value : --value;
    break;
  }
  default:
    return EclStep::Halt;
  }
  return EclStep::Advance;
}

void EnemySystem::CheckInterrupts(EnemyActor *actor) {
  const auto trigger = [actor](const EclInterruptState &interrupt) {
    actor->script.position = *interrupt.target;
    actor->script.wait_counter = 0;
    actor->script.loop_counter = 0;
    actor->t_rep = 0;
  };

  for (size_t index = 0; index < actor->script.interrupts.size(); ++index) {
    const auto &interrupt = actor->script.interrupts[index];
    if (!interrupt.target) {
      continue;
    }

    const auto kind = static_cast<EclInterrupt>(index);
    bool should_trigger = false;
    switch (kind) {
    case EclInterrupt::BossCount:
      should_trigger = bosses_.ActiveCount() <= interrupt.threshold;
      break;
    case EclInterrupt::Hp:
      should_trigger = actor->hp <= interrupt.threshold;
      break;
    case EclInterrupt::Timer:
      should_trigger = actor->script.interrupt_timer > interrupt.threshold;
      if (!should_trigger) {
        ++actor->script.interrupt_timer;
      }
      break;
    case EclInterrupt::BitCount:
      should_trigger = bosses_.BitCount() <= interrupt.threshold;
      break;
    }

    if (should_trigger) {
      trigger(interrupt);
      if (kind == EclInterrupt::Timer) {
        actor->script.interrupt_timer = 0;
      }
      return;
    }
  }
}

uint32_t EnemySystem::ReadValue(const EnemyActor &actor, EclValue value) {
  if (IsEclRegister(value)) {
    return actor.script.registers[RegisterIndex(value)];
  }
  switch (value) {
  case EclValue::LaserAngle:
    return actor.l_cmd.d;
  case EclValue::LaserAngleDelta:
    return actor.l_cmd.dw;
  case EclValue::LaserCount:
    return actor.l_cmd.n;
  case EclValue::LaserColor:
    return actor.l_cmd.c;
  case EclValue::LaserLength:
    return actor.l_cmd.l;
  case EclValue::LaserSpeed:
    return actor.l_cmd.v;
  case EclValue::BulletAngle:
    return actor.t_cmd.d;
  case EclValue::BulletAngleDelta:
    return actor.t_cmd.dw;
  case EclValue::BulletCount:
    return actor.t_cmd.n;
  case EclValue::BulletRapidCount:
    return actor.t_cmd.ns;
  case EclValue::BulletSpeed:
    return actor.t_cmd.v;
  case EclValue::BulletColor:
    return actor.t_cmd.c;
  case EclValue::BulletAcceleration:
    return actor.t_cmd.a;
  case EclValue::BulletRepeat:
    return actor.t_cmd.rep;
  case EclValue::BulletAngularVelocity:
    return actor.t_cmd.vd;
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

void EnemySystem::WriteValue(EnemyActor &actor, EclValue destination,
                             uint32_t value) {
  if (IsEclRegister(destination)) {
    actor.script.registers[RegisterIndex(destination)] = value;
    return;
  }
  switch (destination) {
  case EclValue::LaserAngle:
    actor.l_cmd.d = value;
    break;
  case EclValue::LaserAngleDelta:
    actor.l_cmd.dw = value;
    break;
  case EclValue::LaserCount:
    actor.l_cmd.n = value;
    break;
  case EclValue::LaserColor:
    actor.l_cmd.c = value;
    break;
  case EclValue::LaserLength:
    actor.l_cmd.l = value;
    break;
  case EclValue::LaserSpeed:
    actor.l_cmd.v = value;
    break;
  case EclValue::BulletAngle:
    actor.t_cmd.d = value;
    break;
  case EclValue::BulletAngleDelta:
    actor.t_cmd.dw = value;
    break;
  case EclValue::BulletCount:
    actor.t_cmd.n = value;
    break;
  case EclValue::BulletRapidCount:
    actor.t_cmd.ns = value;
    break;
  case EclValue::BulletSpeed:
    actor.t_cmd.v = value;
    break;
  case EclValue::BulletColor:
    actor.t_cmd.c = value;
    break;
  case EclValue::BulletAcceleration:
    actor.t_cmd.a = value;
    break;
  case EclValue::BulletRepeat:
    actor.t_cmd.rep = value;
    break;
  case EclValue::BulletAngularVelocity:
    actor.t_cmd.vd = value;
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
