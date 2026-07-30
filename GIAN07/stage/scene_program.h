///
/// SceneProgram - validated SCL instructions and runtime cursor
///
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace stage {

enum class SceneOpcode : uint8_t {
  Time = 0x00,
  Enemy = 0x01,
  ScrollSpeed = 0x02,
  Effect = 0x03,
  End = 0x04,
  Boss = 0x05,
  MessageOpen = 0x06,
  MessageClose = 0x07,
  Message = 0x08,
  KeyWait = 0x09,
  NewPage = 0x0a,
  Face = 0x0b,
  Music = 0x0c,
  BossDead = 0x0d,
  LoadFace = 0x0e,
  Wait = 0x0f,
  StageClear = 0x10,
  MapPalette = 0x11,
  GameClear = 0x12,
  DeleteEnemies = 0x13,
  EnemyPalette = 0x14,
  Staff = 0x15,
  ExtraClear = 0x16,
  MessageReference = 0x17,
};

enum class SceneEffect : uint8_t {
  Warning = 0x00,
  EndingFlash = Warning,
  StopWarning = 0x01,
  FadeMusic = 0x02,
  Stage2Boss = 0x03,
  RasterOn = 0x04,
  RasterOff = 0x05,
  CircleFadeIn = 0x06,
  CircleFadeOut = 0x07,
  Stage3Boss = 0x08,
  Stage3Reset = 0x09,
  Stage6Cube = 0x0a,
  Stage6RandomEcl = 0x0b,
  Stage4Rock = 0x0c,
  Stage4Leave = 0x0d,
  WhiteIn = 0x0e,
  WhiteOut = 0x0f,
  LoadExtraBoss1 = 0x10,
  LoadExtraBoss2 = 0x11,
  Stage6Raster = 0x12,
};

enum class SceneWaitCondition : uint8_t {
  BossCount = 0x00,
  BossHp = 0x01,
};

struct SceneInstruction {
  SceneOpcode opcode{};
  int32_t value = 0;
  int16_t x = 0;
  int16_t y = 0;
  uint8_t script_id = 0;
  uint8_t face_id = 0;
  uint8_t track_id = 0;
  uint8_t staff_id = 0;
  uint8_t surface_id = 0;
  uint8_t file_id = 0;
  uint32_t text_id = 0;
  SceneEffect effect = SceneEffect::Warning;
  SceneWaitCondition wait_condition = SceneWaitCondition::BossCount;
  std::string_view text;
};

class SceneProgram {
public:
  [[nodiscard]] static std::optional<SceneProgram>
  Parse(std::span<const uint8_t> bytes);

  [[nodiscard]] const std::vector<SceneInstruction> &Instructions() const {
    return instructions_;
  }

private:
  std::vector<SceneInstruction> instructions_;
};

class SceneRunner {
public:
  [[nodiscard]] bool Load(std::span<const uint8_t> bytes);

  [[nodiscard]] const SceneInstruction *Current() const;
  [[nodiscard]] const std::vector<SceneInstruction> &Instructions() const {
    return program_.Instructions();
  }
  [[nodiscard]] size_t Position() const { return position_; }
  [[nodiscard]] uint32_t Frame() const { return frame_; }
  [[nodiscard]] bool MessageActive() const { return message_active_; }
  [[nodiscard]] bool ReturnLatched() const { return return_latched_; }

  [[nodiscard]] bool TimeReady(uint32_t target, bool skip_pressed);
  [[nodiscard]] bool KeyReady(bool pressed);

  void Advance() { ++position_; }
  void AdvanceFrame() { ++frame_; }
  void SetFrame(uint32_t frame) { frame_ = frame; }
  void SetMessageActive(bool active) { message_active_ = active; }
  void SetReturnLatched(bool latched) { return_latched_ = latched; }

private:
  SceneProgram program_;
  size_t position_ = 0;
  uint32_t frame_ = 0;
  int key_wait_count_ = 0;
  bool message_active_ = false;
  bool return_latched_ = false;
};

} // namespace stage
