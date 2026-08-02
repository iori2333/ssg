///
/// BossHealthGauge - boss health and timeout HUD state
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "enemy/boss/boss.h"

namespace audio {
class AudioSystem;
}

inline constexpr std::size_t kBossHealthGaugeHeight = 24;

class BossHealthGauge {
public:
  explicit BossHealthGauge(audio::AudioSystem &audio) : audio_(audio) {}

  void Reset();
  void Sync(const BossHudModel &model);
  void Open(uint32_t max_hp);
  void AddPhase(uint32_t next);
  void Update(uint32_t now);
  void Draw(int stage_frame);
  void SetCombatState(int phase_threshold_hp, int timer_max, int timer_now);
  void SetStageTimeout(int timeout_end);

private:
  audio::AudioSystem &audio_;

  enum class State : uint8_t {
    Hidden,
    OpeningFrame,
    Filling,
    Ready,
    Closing,
    Refilling
  };

  void Close();

  uint32_t current_hp_ = 0;
  uint32_t max_hp_ = 0;
  uint32_t target_hp_ = 0;
  uint32_t phase_hp_ = 0;
  std::array<int, kBossHealthGaugeHeight> row_x_{};
  State state_ = State::Hidden;
  int phase_threshold_hp_ = -1;
  int timer_max_ = -1;
  int timer_now_ = 0;
  int previous_timer_seconds_ = -1;
  int stage_timeout_end_ = -1;
  uint64_t encounter_revision_ = 0;
  uint64_t phase_revision_ = 0;
};
