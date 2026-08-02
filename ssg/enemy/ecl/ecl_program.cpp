///
/// EclProgram - validated, decoded enemy script program
///

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ecl.h"
#include "ecl_program.h"

#include "effect/effect_types.h"
#include "stage/stage_visuals.h"
#include "util/byte_io.h"

class EclInstructionFactory {
public:
  template <typename Arguments>
  static EclInstruction Make(EclOpcode opcode, Arguments arguments) {
    return EclInstruction(opcode, std::move(arguments));
  }
};

namespace {

constexpr size_t InvalidPosition = std::numeric_limits<size_t>::max();

class EclReader {
public:
  explicit EclReader(std::span<const uint8_t> bytes) : reader_(bytes) {}

  template <typename T> T ReadRequired() {
    auto value = reader_.Read<T>();
    if (!value) {
      throw std::runtime_error("Unexpected end of ECL program");
    }
    return *value;
  }

  uint8_t U8() { return ReadRequired<uint8_t>(); }
  int8_t I8() { return std::bit_cast<int8_t>(U8()); }

  uint16_t U16() { return ReadRequired<uint16_t>(); }

  int16_t I16() { return ReadRequired<int16_t>(); }

  uint32_t U32() { return ReadRequired<uint32_t>(); }

  int32_t I32() { return ReadRequired<int32_t>(); }

private:
  util::ByteReader reader_;
};

std::optional<size_t> EncodedSize(uint8_t raw_opcode) {
  const auto opcode = static_cast<EclOpcode>(raw_opcode);
  switch (opcode) {
  case EclOpcode::End:
  case EclOpcode::Return:
  case EclOpcode::RandomAngle:
  case EclOpcode::AimAtPlayer:
  case EclOpcode::RandomAngleUp:
  case EclOpcode::RandomAngleDown:
  case EclOpcode::SetSequenceAngle:
  case EclOpcode::MoveToPlayerPosition:
  case EclOpcode::RandomBoundedAngle:
  case EclOpcode::RandomPosition:
  case EclOpcode::FireBullet:
  case EclOpcode::AimBulletAtPlayer:
  case EclOpcode::SyncBulletAngle:
  case EclOpcode::FireBulletUnscaled:
  case EclOpcode::ClearBullets:
  case EclOpcode::FireBulletLine:
  case EclOpcode::FireExtraBullet:
  case EclOpcode::FireLaser:
  case EclOpcode::AimLaserAtPlayer:
  case EclOpcode::SyncLaserAngle:
  case EclOpcode::FireLaserUnscaled:
  case EclOpcode::SpawnLongLaser:
  case EclOpcode::FireHomingLaser:
  case EclOpcode::EnableDraw:
  case EclOpcode::DisableDraw:
  case EclOpcode::EnableClip:
  case EclOpcode::DisableClip:
  case EclOpcode::EnableDamage:
  case EclOpcode::DisableDamage:
  case EclOpcode::EnablePlayerCollision:
  case EclOpcode::DisablePlayerCollision:
  case EclOpcode::EnableHorizontalMirror:
  case EclOpcode::DisableHorizontalMirror:
  case EclOpcode::Stage3Effect:
    return 1;

  case EclOpcode::ClearInterrupt:
  case EclOpcode::GravityBounce:
  case EclOpcode::SetAngle:
  case EclOpcode::AddAngle:
  case EclOpcode::SetAutoFire:
  case EclOpcode::SetBulletCommand:
  case EclOpcode::SetBulletOption:
  case EclOpcode::SetBulletType:
  case EclOpcode::SetBulletColor:
  case EclOpcode::SetBulletAngularVelocity:
  case EclOpcode::SetBulletRepeat:
  case EclOpcode::BulletsToItems:
  case EclOpcode::SetLaserCommand:
  case EclOpcode::SetLaserCount:
  case EclOpcode::AddLaserCount:
  case EclOpcode::SetLaserColor:
  case EclOpcode::SetLaserType:
  case EclOpcode::OpenLongLaser:
  case EclOpcode::CloseLongLaser:
  case EclOpcode::CloseLongLaserToLine:
  case EclOpcode::PlaySound:
  case EclOpcode::BossAction:
  case EclOpcode::SetSequenceAngleDelta:
  case EclOpcode::SetItem:
  case EclOpcode::Stage4Effect:
  case EclOpcode::SetDamageAnimation:
  case EclOpcode::BitLaser:
  case EclOpcode::SpawnBoss:
  case EclOpcode::Random:
  case EclOpcode::Increment:
  case EclOpcode::Decrement:
    return 2;

  case EclOpcode::Wait:
  case EclOpcode::WaitScroll:
  case EclOpcode::Move:
  case EclOpcode::MoveToPlayerX:
  case EclOpcode::MoveToPlayerY:
  case EclOpcode::MoveToPlayer:
  case EclOpcode::MovePolar:
  case EclOpcode::SetBulletAngle:
  case EclOpcode::AddBulletAngle:
  case EclOpcode::SetBulletCount:
  case EclOpcode::AddBulletCount:
  case EclOpcode::SetBulletSpeed:
  case EclOpcode::AddBulletSpeed:
  case EclOpcode::SetLaserAngle:
  case EclOpcode::AddLaserAngle:
  case EclOpcode::AddLongLaserAngle:
  case EclOpcode::SetAnimation:
  case EclOpcode::MoveValue:
  case EclOpcode::AddValue:
  case EclOpcode::SubtractValue:
  case EclOpcode::Sine:
  case EclOpcode::Cosine:
  case EclOpcode::CompareRegisters:
    return 3;

  case EclOpcode::Rotate:
  case EclOpcode::Accelerate:
    return 4;

  case EclOpcode::Jump:
  case EclOpcode::Call:
  case EclOpcode::JumpDirection:
  case EclOpcode::MoveX:
  case EclOpcode::MoveY:
  case EclOpcode::SetSpeed:
  case EclOpcode::AddSpeed:
  case EclOpcode::SetPosition:
  case EclOpcode::AddPosition:
  case EclOpcode::SetBulletOffset:
  case EclOpcode::SetLaserLength:
  case EclOpcode::AddLaserLength:
  case EclOpcode::SetLaserStartLength:
  case EclOpcode::SetLaserSpeed:
  case EclOpcode::AddLaserSpeed:
  case EclOpcode::SetLaserWidth:
  case EclOpcode::SetLaserOffset:
  case EclOpcode::SetHitbox:
  case EclOpcode::BitAttack:
  case EclOpcode::JumpGreater:
  case EclOpcode::JumpLess:
  case EclOpcode::JumpEqual:
    return 5;

  case EclOpcode::SpawnEnemy:
  case EclOpcode::BitCommand:
  case EclOpcode::SpawnCircleEffect:
  case EclOpcode::SetRegister:
  case EclOpcode::Modulo:
  case EclOpcode::CompareConstant:
    return 6;

  case EclOpcode::Loop:
  case EclOpcode::MoveXY:
  case EclOpcode::AccelerateTo:
  case EclOpcode::SpawnEnemyWithAngle:
    return 7;

  case EclOpcode::Setup:
  case EclOpcode::JumpHpGreater:
  case EclOpcode::JumpHpLess:
  case EclOpcode::JumpFrameGreater:
  case EclOpcode::JumpFrameLess:
  case EclOpcode::WaveX:
  case EclOpcode::WaveY:
    return 9;

  case EclOpcode::SetInterrupt:
    return 10;

  case EclOpcode::LinearRotate:
    return 12;

  case EclOpcode::JumpDifficulty:
    return 17;
  }
  return std::nullopt;
}

std::optional<EclValue> DecodeValue(uint8_t raw) {
  if (raw < kEclRegisterCount ||
      (raw >= static_cast<uint8_t>(EclValue::LaserAngle) &&
       raw <= static_cast<uint8_t>(EclValue::ActorAngle))) {
    return static_cast<EclValue>(raw);
  }
  return std::nullopt;
}

std::optional<EclValue> DecodeRegister(uint8_t raw) {
  const auto value = DecodeValue(raw);
  return value && IsEclRegister(*value) ? value : std::nullopt;
}

template <typename Arguments>
EclInstruction MakeInstruction(EclOpcode opcode, Arguments arguments) {
  return EclInstructionFactory::Make(opcode, std::move(arguments));
}

std::optional<EclInstruction>
DecodeInstruction(std::span<const uint8_t> bytes, size_t address,
                  const std::vector<size_t> &address_to_position,
                  size_t script_count) {
  const auto opcode = static_cast<EclOpcode>(bytes[address]);
  EclReader reader(bytes.subspan(address + 1));
  const auto target = [&]() -> std::optional<size_t> {
    const auto byte_address = reader.U32();
    if (byte_address >= address_to_position.size() ||
        address_to_position[byte_address] == InvalidPosition) {
      return std::nullopt;
    }
    return address_to_position[byte_address];
  };
  const auto script = [&](uint32_t script_id) {
    return script_id < script_count;
  };

  switch (opcode) {
  case EclOpcode::Setup:
    return MakeInstruction(
        opcode, EclSetupArguments{.hp = reader.U32(), .score = reader.U32()});

  case EclOpcode::End:
  case EclOpcode::Return:
  case EclOpcode::RandomAngle:
  case EclOpcode::AimAtPlayer:
  case EclOpcode::RandomAngleUp:
  case EclOpcode::RandomAngleDown:
  case EclOpcode::SetSequenceAngle:
  case EclOpcode::MoveToPlayerPosition:
  case EclOpcode::RandomBoundedAngle:
  case EclOpcode::RandomPosition:
  case EclOpcode::FireBullet:
  case EclOpcode::AimBulletAtPlayer:
  case EclOpcode::SyncBulletAngle:
  case EclOpcode::FireBulletUnscaled:
  case EclOpcode::ClearBullets:
  case EclOpcode::FireBulletLine:
  case EclOpcode::FireExtraBullet:
  case EclOpcode::FireLaser:
  case EclOpcode::AimLaserAtPlayer:
  case EclOpcode::SyncLaserAngle:
  case EclOpcode::FireLaserUnscaled:
  case EclOpcode::SpawnLongLaser:
  case EclOpcode::FireHomingLaser:
  case EclOpcode::EnableDraw:
  case EclOpcode::DisableDraw:
  case EclOpcode::EnableClip:
  case EclOpcode::DisableClip:
  case EclOpcode::EnableDamage:
  case EclOpcode::DisableDamage:
  case EclOpcode::EnablePlayerCollision:
  case EclOpcode::DisablePlayerCollision:
  case EclOpcode::EnableHorizontalMirror:
  case EclOpcode::DisableHorizontalMirror:
  case EclOpcode::Stage3Effect:
    return MakeInstruction(opcode, EclNoArguments{});

  case EclOpcode::Jump:
  case EclOpcode::Call:
  case EclOpcode::JumpDirection:
  case EclOpcode::JumpGreater:
  case EclOpcode::JumpLess:
  case EclOpcode::JumpEqual: {
    const auto position = target();
    if (!position) {
      return std::nullopt;
    }
    return MakeInstruction(opcode, EclJumpArguments{.target = *position});
  }

  case EclOpcode::Loop: {
    const auto position = target();
    if (!position) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclLoopArguments{.target = *position, .count = reader.U16()});
  }

  case EclOpcode::JumpHpGreater:
  case EclOpcode::JumpHpLess:
  case EclOpcode::JumpFrameGreater:
  case EclOpcode::JumpFrameLess: {
    const auto position = target();
    if (!position) {
      return std::nullopt;
    }
    return MakeInstruction(opcode,
                           EclConditionalJumpArguments{.target = *position,
                                                       .value = static_cast<int>(reader.U32())});
  }

  case EclOpcode::JumpDifficulty: {
    EclDifficultyJumpArguments arguments{};
    for (auto &position : arguments.targets) {
      const auto decoded = target();
      if (!decoded) {
        return std::nullopt;
      }
      position = *decoded;
    }
    return MakeInstruction(opcode, arguments);
  }

  case EclOpcode::SetInterrupt: {
    const auto position = target();
    const auto raw_interrupt = reader.U8();
    if (!position || raw_interrupt >= kEclInterruptCount) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclSetInterruptArguments{
                    .target = *position,
                    .interrupt = static_cast<EclInterrupt>(raw_interrupt),
                    .threshold = static_cast<int>(reader.U32())});
  }

  case EclOpcode::ClearInterrupt: {
    const auto raw_interrupt = reader.U8();
    if (raw_interrupt >= kEclInterruptCount) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclInterruptArguments{
                    .interrupt = static_cast<EclInterrupt>(raw_interrupt)});
  }

  case EclOpcode::Wait:
  case EclOpcode::WaitScroll:
  case EclOpcode::Move:
  case EclOpcode::MoveToPlayerX:
  case EclOpcode::MoveToPlayerY:
  case EclOpcode::MoveToPlayer:
    return MakeInstruction(opcode,
                           EclDurationArguments{.frames = reader.U16()});

  case EclOpcode::Rotate:
    return MakeInstruction(
        opcode,
        EclRotateArguments{.angle_delta = reader.I8(), .frames = reader.U16()});

  case EclOpcode::LinearRotate:
    return MakeInstruction(opcode,
                           EclLinearRotateArguments{.velocity_x = reader.I32(),
                                                    .velocity_y = reader.I32(),
                                                    .angle_delta = reader.I8(),
                                                    .frames = reader.U16()});

  case EclOpcode::WaveX:
  case EclOpcode::WaveY:
    return MakeInstruction(opcode, EclWaveArguments{.velocity = reader.I32(),
                                                    .amplitude = reader.U8(),
                                                    .angle_delta = reader.I8(),
                                                    .frames = reader.U16()});

  case EclOpcode::MoveX:
  case EclOpcode::MoveY:
    return MakeInstruction(opcode,
                           EclAxisMoveArguments{.coordinate = reader.U16(),
                                                .frames = reader.U16()});

  case EclOpcode::MoveXY:
    return MakeInstruction(opcode,
                           EclPointMoveArguments{.x = reader.U16(),
                                                 .y = reader.U16(),
                                                 .frames = reader.U16()});

  case EclOpcode::Accelerate:
    return MakeInstruction(opcode,
                           EclAccelerationArguments{.acceleration = reader.I8(),
                                                    .frames = reader.U16()});

  case EclOpcode::AccelerateTo:
    return MakeInstruction(
        opcode, EclAccelerationPointArguments{.x = reader.I16(),
                                              .y = reader.I16(),
                                              .velocity = reader.I16()});

  case EclOpcode::GravityBounce:
  case EclOpcode::AddAngle:
  case EclOpcode::SetBulletAngularVelocity:
  case EclOpcode::AddLaserCount:
    return MakeInstruction(opcode,
                           EclSignedByteArguments{.value = reader.I8()});

  case EclOpcode::SetAngle:
  case EclOpcode::SetAutoFire:
  case EclOpcode::SetBulletCommand:
  case EclOpcode::SetBulletOption:
  case EclOpcode::SetBulletType:
  case EclOpcode::SetBulletColor:
  case EclOpcode::SetBulletRepeat:
  case EclOpcode::BulletsToItems:
  case EclOpcode::SetLaserCommand:
  case EclOpcode::SetLaserCount:
  case EclOpcode::SetLaserColor:
  case EclOpcode::SetLaserType:
  case EclOpcode::PlaySound:
  case EclOpcode::SetSequenceAngleDelta:
  case EclOpcode::SetItem:
  case EclOpcode::SetDamageAnimation:
    return MakeInstruction(opcode, EclByteArguments{.value = reader.U8()});

  case EclOpcode::Stage4Effect: {
    const auto command = reader.U8();
    if (command > static_cast<uint8_t>(stage::Stage4RockCommand::End)) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclStage4EffectArguments{
                    .command = static_cast<stage::Stage4RockCommand>(command)});
  }

  case EclOpcode::SetSpeed:
  case EclOpcode::AddSpeed:
  case EclOpcode::SetLaserLength:
  case EclOpcode::AddLaserLength:
  case EclOpcode::SetLaserStartLength:
  case EclOpcode::SetLaserSpeed:
  case EclOpcode::AddLaserSpeed:
  case EclOpcode::SetLaserWidth:
    return MakeInstruction(opcode,
                           EclSignedDwordArguments{.value = reader.I32()});

  case EclOpcode::SetPosition:
  case EclOpcode::AddPosition:
  case EclOpcode::SetBulletOffset:
  case EclOpcode::SetLaserOffset:
    return MakeInstruction(
        opcode, EclPointArguments{.x = reader.I16(), .y = reader.I16()});

  case EclOpcode::MovePolar:
    return MakeInstruction(opcode,
                           EclSignedWordArguments{.value = reader.I16()});

  case EclOpcode::SetBulletAngle:
  case EclOpcode::SetBulletCount:
  case EclOpcode::SetLaserAngle:
    return MakeInstruction(opcode, EclBytePairArguments{.first = reader.U8(),
                                                        .second = reader.U8()});

  case EclOpcode::AddBulletAngle:
  case EclOpcode::AddBulletCount:
  case EclOpcode::AddBulletSpeed:
  case EclOpcode::AddLaserAngle:
    return MakeInstruction(opcode,
                           EclSignedBytePairArguments{.first = reader.I8(),
                                                      .second = reader.I8()});

  case EclOpcode::SetBulletSpeed:
    return MakeInstruction(opcode,
                           EclByteSignedByteArguments{.first = reader.U8(),
                                                      .second = reader.I8()});

  case EclOpcode::OpenLongLaser:
  case EclOpcode::CloseLongLaser:
  case EclOpcode::CloseLongLaserToLine:
    return MakeInstruction(opcode, EclLongLaserArguments{.id = reader.U8()});

  case EclOpcode::AddLongLaserAngle:
    return MakeInstruction(
        opcode,
        EclLongLaserArguments{.id = reader.U8(), .angle_delta = reader.I8()});

  case EclOpcode::SetAnimation:
    return MakeInstruction(opcode, EclAnimationArguments{.pattern = reader.U8(),
                                                         .speed = reader.I8()});

  case EclOpcode::BossAction: {
    const auto raw_action = reader.U8();
    if (raw_action > static_cast<uint8_t>(EclBossAction::BombSpirit)) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclBossActionArguments{
                    .action = static_cast<EclBossAction>(raw_action)});
  }

  case EclOpcode::SpawnEnemy:
  case EclOpcode::SpawnEnemyWithAngle: {
    EclSpawnEnemyArguments arguments{.offset_x = reader.I16(),
                                     .offset_y = reader.I16()};
    if (opcode == EclOpcode::SpawnEnemyWithAngle) {
      arguments.angle_source = DecodeValue(reader.U8());
      if (!arguments.angle_source) {
        return std::nullopt;
      }
    }
    arguments.script_id = reader.U8();
    if (!script(arguments.script_id)) {
      return std::nullopt;
    }
    return MakeInstruction(opcode, arguments);
  }

  case EclOpcode::SetHitbox:
    return MakeInstruction(opcode, EclHitboxArguments{.width = reader.U16(),
                                                      .height = reader.U16()});

  case EclOpcode::BitLaser: {
    const auto raw_command = reader.U8();
    if (raw_command > static_cast<uint8_t>(EclBitLaserCommand::Star)) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclBitLaserArguments{
                    .command = static_cast<EclBitLaserCommand>(raw_command)});
  }

  case EclOpcode::BitAttack: {
    const auto script_id = reader.U32();
    if (!script(script_id)) {
      return std::nullopt;
    }
    return MakeInstruction(opcode, EclScriptArguments{.script_id = script_id});
  }

  case EclOpcode::BitCommand: {
    const auto raw_command = reader.U8();
    if (raw_command < static_cast<uint8_t>(EclBitCommand::ChangeSpeed) ||
        raw_command > static_cast<uint8_t>(EclBitCommand::MoveTowardPlayer)) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclCommandValueArguments{
                    .command = static_cast<EclBitCommand>(raw_command),
                    .value = reader.I32()});
  }

  case EclOpcode::SpawnBoss: {
    const auto script_id = reader.U8();
    if (!script(script_id)) {
      return std::nullopt;
    }
    return MakeInstruction(opcode, EclScriptArguments{.script_id = script_id});
  }

  case EclOpcode::SpawnCircleEffect: {
    const auto offset_x = reader.I16();
    const auto offset_y = reader.I16();
    const auto effect = reader.U8();
    if (effect < static_cast<uint8_t>(CircleEffectKind::Star) ||
        effect > static_cast<uint8_t>(CircleEffectKind::Diverging)) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclCircleEffectArguments{
                    .offset_x = offset_x,
                    .offset_y = offset_y,
                    .effect = static_cast<CircleEffectKind>(effect)});
  }

  case EclOpcode::MoveValue: {
    const auto destination = DecodeValue(reader.U8());
    const auto source = DecodeValue(reader.U8());
    if (!destination || !source) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode,
        EclMoveValueArguments{.destination = *destination, .source = *source});
  }

  case EclOpcode::SetRegister: {
    const auto destination = DecodeRegister(reader.U8());
    if (!destination) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclRegisterConstantArguments{.destination = *destination,
                                             .value = reader.U32()});
  }

  case EclOpcode::AddValue:
  case EclOpcode::SubtractValue: {
    const auto destination = DecodeRegister(reader.U8());
    const auto source = DecodeValue(reader.U8());
    if (!destination || !source) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclRegisterValueArguments{.destination = *destination,
                                          .source = *source});
  }

  case EclOpcode::Sine:
  case EclOpcode::Cosine:
  case EclOpcode::CompareRegisters: {
    const auto first = DecodeRegister(reader.U8());
    const auto second = DecodeRegister(reader.U8());
    if (!first || !second) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclRegisterPairArguments{.first = *first, .second = *second});
  }

  case EclOpcode::Modulo: {
    const auto destination = DecodeRegister(reader.U8());
    if (!destination) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclRegisterConstantArguments{.destination = *destination,
                                             .value = reader.U32()});
  }

  case EclOpcode::Random:
  case EclOpcode::Increment:
  case EclOpcode::Decrement: {
    const auto destination = DecodeRegister(reader.U8());
    if (!destination) {
      return std::nullopt;
    }
    return MakeInstruction(opcode,
                           EclRegisterArguments{.destination = *destination});
  }

  case EclOpcode::CompareConstant: {
    const auto destination = DecodeRegister(reader.U8());
    if (!destination) {
      return std::nullopt;
    }
    return MakeInstruction(
        opcode, EclRegisterSignedConstantArguments{.destination = *destination,
                                                   .value = reader.I32()});
  }
  }
  return std::nullopt;
}

} // namespace

std::optional<EclProgram> EclProgram::Parse(std::span<const uint8_t> bytes) {
  if (bytes.size() < sizeof(uint32_t)) {
    return std::nullopt;
  }

  const auto script_count_value = util::ReadLittleAt<uint32_t>(bytes, 0);
  if (!script_count_value || *script_count_value == 0 ||
      *script_count_value > 256) {
    return std::nullopt;
  }
  const auto script_count = *script_count_value;

  const size_t header_size =
      sizeof(uint32_t) + static_cast<size_t>(script_count) * sizeof(uint32_t);
  if (header_size > bytes.size()) {
    return std::nullopt;
  }

  std::vector<size_t> addresses;
  std::vector<size_t> address_to_position(bytes.size(), InvalidPosition);
  for (size_t address = header_size; address < bytes.size();) {
    const auto size = EncodedSize(bytes[address]);
    if (!size || *size > bytes.size() - address) {
      return std::nullopt;
    }
    address_to_position[address] = addresses.size();
    addresses.push_back(address);
    address += *size;
    if (address > bytes.size()) {
      return std::nullopt;
    }
  }
  if (addresses.empty()) {
    return std::nullopt;
  }
  const auto last_address = addresses.back();
  const auto last_size = EncodedSize(bytes[last_address]);
  if (!last_size || last_address + *last_size != bytes.size()) {
    return std::nullopt;
  }

  EclProgram program;
  program.entries_.reserve(script_count);
  for (uint32_t i = 0; i < script_count; ++i) {
    const auto entry_address = util::ReadLittleAt<uint32_t>(
        bytes, sizeof(uint32_t) + i * sizeof(uint32_t));
    if (!entry_address || *entry_address >= address_to_position.size() ||
        address_to_position[*entry_address] == InvalidPosition) {
      return std::nullopt;
    }
    program.entries_.push_back(address_to_position[*entry_address]);
  }

  program.instructions_.reserve(addresses.size());
  for (const auto address : addresses) {
    auto instruction = DecodeInstruction(bytes, address, address_to_position,
                                         program.entries_.size());
    if (!instruction) {
      return std::nullopt;
    }
    program.instructions_.push_back(*instruction);
  }
  return program;
}

std::optional<size_t> EclProgram::Entry(uint32_t script_id) const {
  if (script_id >= entries_.size()) {
    return std::nullopt;
  }
  return entries_[script_id];
}

const EclInstruction *EclProgram::InstructionAt(size_t position) const {
  return position < instructions_.size() ? &instructions_[position] : nullptr;
}
