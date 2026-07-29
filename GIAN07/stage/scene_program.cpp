///
/// SceneProgram - validated SCL instructions and runtime cursor
///
#include <algorithm>
#include <limits>
#include <utility>

#include "scene_program.h"

#include "util/endian.h"

namespace {

class SceneReader {
public:
  explicit SceneReader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

  [[nodiscard]] bool Empty() const { return offset_ == bytes_.size(); }

  [[nodiscard]] std::optional<uint8_t> ReadU8() {
    if (offset_ >= bytes_.size()) {
      return std::nullopt;
    }
    return bytes_[offset_++];
  }

  [[nodiscard]] std::optional<int16_t> ReadI16() {
    if (bytes_.size() - offset_ < sizeof(int16_t)) {
      return std::nullopt;
    }
    const auto value = I16LEAt(bytes_.data() + offset_);
    offset_ += sizeof(int16_t);
    return value;
  }

  [[nodiscard]] std::optional<uint32_t> ReadU32() {
    if (bytes_.size() - offset_ < sizeof(uint32_t)) {
      return std::nullopt;
    }
    const auto value = U32LEAt(bytes_.data() + offset_);
    offset_ += sizeof(uint32_t);
    return value;
  }

  [[nodiscard]] std::optional<std::string_view> ReadString() {
    const auto begin = bytes_.begin() + static_cast<ptrdiff_t>(offset_);
    const auto end = std::ranges::find(begin, bytes_.end(), uint8_t{0});
    if (end == bytes_.end()) {
      return std::nullopt;
    }
    const auto length = static_cast<size_t>(end - begin);
    const auto *text = reinterpret_cast<const char *>(bytes_.data() + offset_);
    offset_ += length + 1;
    return std::string_view(text, length);
  }

private:
  std::span<const uint8_t> bytes_;
  size_t offset_ = 0;
};

} // namespace

namespace stage {

std::optional<SceneProgram>
SceneProgram::Parse(std::span<const uint8_t> bytes) {
  if (bytes.empty()) {
    return std::nullopt;
  }

  SceneProgram program;
  SceneReader reader(bytes);
  while (!reader.Empty()) {
    const auto raw_opcode = reader.ReadU8();
    if (!raw_opcode ||
        *raw_opcode > std::to_underlying(SceneOpcode::ExtraClear)) {
      return std::nullopt;
    }

    SceneInstruction instruction{.opcode =
                                     static_cast<SceneOpcode>(*raw_opcode)};
    switch (instruction.opcode) {
    case SceneOpcode::Time: {
      const auto value = reader.ReadU32();
      if (!value || *value > std::numeric_limits<int32_t>::max()) {
        return std::nullopt;
      }
      instruction.value = static_cast<int32_t>(*value);
      break;
    }
    case SceneOpcode::Enemy:
    case SceneOpcode::Boss: {
      const auto x = reader.ReadI16();
      const auto y = reader.ReadI16();
      const auto id = reader.ReadU8();
      if (!x || !y || !id) {
        return std::nullopt;
      }
      instruction.x = *x;
      instruction.y = *y;
      instruction.script_id = *id;
      break;
    }
    case SceneOpcode::ScrollSpeed: {
      const auto speed = reader.ReadI16();
      if (!speed) {
        return std::nullopt;
      }
      instruction.value = *speed;
      break;
    }
    case SceneOpcode::Face: {
      const auto face = reader.ReadU8();
      if (!face) {
        return std::nullopt;
      }
      instruction.face_id = *face;
      break;
    }
    case SceneOpcode::Music: {
      const auto track = reader.ReadU8();
      if (!track) {
        return std::nullopt;
      }
      instruction.track_id = *track;
      break;
    }
    case SceneOpcode::Staff: {
      const auto staff = reader.ReadU8();
      if (!staff) {
        return std::nullopt;
      }
      instruction.staff_id = *staff;
      break;
    }
    case SceneOpcode::Effect: {
      const auto effect = reader.ReadU8();
      if (!effect || *effect > std::to_underlying(SceneEffect::Stage6Raster)) {
        return std::nullopt;
      }
      instruction.effect = static_cast<SceneEffect>(*effect);
      break;
    }
    case SceneOpcode::Message: {
      const auto text = reader.ReadString();
      if (!text) {
        return std::nullopt;
      }
      instruction.text = *text;
      break;
    }
    case SceneOpcode::LoadFace: {
      const auto surface = reader.ReadU8();
      const auto file = reader.ReadU8();
      if (!surface || !file) {
        return std::nullopt;
      }
      instruction.surface_id = *surface;
      instruction.file_id = *file;
      break;
    }
    case SceneOpcode::Wait: {
      const auto condition = reader.ReadU8();
      const auto option = reader.ReadU32();
      if (!condition || !option ||
          *condition > std::to_underlying(SceneWaitCondition::BossHp) ||
          *option > std::numeric_limits<int32_t>::max()) {
        return std::nullopt;
      }
      instruction.wait_condition = static_cast<SceneWaitCondition>(*condition);
      instruction.value = static_cast<int32_t>(*option);
      break;
    }
    case SceneOpcode::End:
    case SceneOpcode::MessageOpen:
    case SceneOpcode::MessageClose:
    case SceneOpcode::KeyWait:
    case SceneOpcode::NewPage:
    case SceneOpcode::BossDead:
    case SceneOpcode::StageClear:
    case SceneOpcode::MapPalette:
    case SceneOpcode::GameClear:
    case SceneOpcode::DeleteEnemies:
    case SceneOpcode::EnemyPalette:
    case SceneOpcode::ExtraClear:
      break;
    }
    program.instructions_.push_back(instruction);
  }

  return program;
}

bool SceneRunner::Load(std::span<const uint8_t> bytes) {
  auto program = SceneProgram::Parse(bytes);
  if (!program) {
    return false;
  }
  program_ = std::move(*program);
  position_ = 0;
  frame_ = 0;
  key_wait_count_ = 0;
  message_active_ = false;
  return_latched_ = false;
  return true;
}

const SceneInstruction *SceneRunner::Current() const {
  const auto &instructions = program_.Instructions();
  return position_ < instructions.size() ? &instructions[position_] : nullptr;
}

bool SceneRunner::TimeReady(uint32_t target, bool skip_pressed) {
  if (message_active_) {
    if (skip_pressed) {
      if (!return_latched_) {
        frame_ = target;
        return_latched_ = true;
      }
    } else {
      return_latched_ = false;
    }
  }
  return target <= frame_;
}

bool SceneRunner::KeyReady(bool pressed) {
  if (pressed || ++key_wait_count_ >= 180) {
    key_wait_count_ = 0;
    return true;
  }
  return false;
}

} // namespace stage
