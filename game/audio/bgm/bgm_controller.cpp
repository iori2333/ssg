#include "bgm_controller.h"
#include "waveform_playback.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <utility>

#include "audio/core/audio_types.h"
#include "audio/midi/midi_parser.h"
#include "audio/midi/midi_sequencer.h"
#include "audio/midi/midi_synth.h"

namespace audio::bgm {
namespace {

constexpr std::uint8_t kMidiChannelCount = 16;
constexpr std::uint8_t kTempoDenominator = 128;

float LinearVolume(Volume volume) {
  return (static_cast<float>(volume) /
          static_cast<float>(kMaxVolume));
}

} // namespace

BgmController::BgmController(ma_engine &engine,
                             midi::MidiSequencer &sequencer,
                             midi::MidiSynth &synth)
    : waveform_(engine), sequencer_(sequencer), synth_(synth) {}

AudioResult BgmController::LoadMidi(midi::SequenceData sequence) {
  waveform_.Unload();
  title_ = sequence.title;
  sequencer_.Load(std::move(sequence));
  sequencer_.Rewind();
  midi_loaded_ = true;
  mode_ = BgmMode::Midi;
  state_ = PlaybackState::Ready;
  midi_fade_remaining_ = {};
  return AudioResult::Ok();
}

AudioResult BgmController::LoadWaveform(std::string_view path) {
  auto result = waveform_.Load(path);
  if (!result.success) {
    return result;
  }
  title_ = std::string{waveform_.Title()};
  mode_ = BgmMode::Waveform;
  state_ = PlaybackState::Ready;
  midi_fade_remaining_ = {};
  UpdateWaveformVolume();
  return AudioResult::Ok();
}

void BgmController::ClearWaveform() {
  waveform_.Unload();
  midi_fade_remaining_ = {};
  if (midi_loaded_) {
    mode_ = BgmMode::Midi;
    state_ = PlaybackState::Ready;
  } else {
    mode_ = BgmMode::None;
    state_ = PlaybackState::Idle;
    title_.clear();
  }
}

void BgmController::Play() {
  if (mode_ == BgmMode::None || state_ == PlaybackState::Faulted) {
    return;
  }
  state_ = PlaybackState::Starting;
  if (mode_ == BgmMode::Waveform) {
    waveform_.Play();
    UpdateWaveformVolume();
  } else if (midi_loaded_) {
    synth_.Resume();
    sequencer_.Rewind();
    synth_.Panic();
    ApplyMidiVolume();
  }
  playing_ = true;
  state_ = PlaybackState::Playing;
}

void BgmController::Stop() {
  playing_ = false;
  waveform_.Stop();
  synth_.Resume();
  synth_.Panic();
  midi_fade_remaining_ = {};
  state_ =
      ((mode_ == BgmMode::None) ? PlaybackState::Idle
                                : PlaybackState::Ready);
}

void BgmController::Pause() {
  if (!playing_) {
    return;
  }
  playing_ = false;
  waveform_.Pause();
  if (mode_ == BgmMode::Midi && midi_loaded_) {
    synth_.Pause();
  }
  state_ = PlaybackState::Paused;
}

void BgmController::Resume() {
  if (playing_ || mode_ == BgmMode::None) {
    return;
  }
  if (mode_ == BgmMode::Waveform) {
    waveform_.Resume();
  } else if (midi_loaded_) {
    synth_.Resume();
  }
  playing_ = true;
  state_ = PlaybackState::Playing;
}

void BgmController::FadeOut(float volume_start,
                            std::chrono::milliseconds duration) {
  if (mode_ == BgmMode::Waveform) {
    waveform_.FadeOut(volume_start, duration);
  } else if (midi_loaded_) {
    midi_fade_start_ = volume_start;
    midi_fade_duration_ = duration;
    midi_fade_remaining_ = duration;
  }
}

void BgmController::SetVolume(Volume volume) {
  volume_ = std::min(volume, kMaxVolume);
  if (mode_ == BgmMode::Waveform) {
    UpdateWaveformVolume();
  } else if (midi_loaded_) {
    ApplyMidiVolume();
  }
}

void BgmController::SetTempo(std::int8_t tempo) {
  tempo_ = std::clamp(tempo, std::int8_t{-100}, std::int8_t{100});
  const auto numerator =
      static_cast<std::uint8_t>(kTempoDenominator + tempo_);
  sequencer_.SetTempo(numerator, kTempoDenominator);
  if (mode_ == BgmMode::Waveform) {
    waveform_.SetPitch(static_cast<float>(numerator) /
                       static_cast<float>(kTempoDenominator));
  }
}

void BgmController::SetGainApplied(bool enabled) {
  gain_applied_ = enabled;
  UpdateWaveformVolume();
}

void BgmController::Tick(std::chrono::milliseconds delta) {
  if (!playing_) {
    return;
  }

  if (mode_ == BgmMode::Waveform) {
    if (waveform_.FadeVolumeLinear() <= 0.0f) {
      Stop();
      return;
    }
    sequencer_.Tick(delta, synth_, false);
    return;
  }

  if (mode_ == BgmMode::Midi && midi_loaded_) {
    sequencer_.Tick(delta, synth_, true);
    if (midi_fade_remaining_ > std::chrono::milliseconds::zero()) {
      midi_fade_remaining_ =
          std::max(midi_fade_remaining_ - delta,
                   std::chrono::milliseconds::zero());
      const auto progress = midi_fade_duration_.count();
      const auto remaining = midi_fade_remaining_.count();
      const auto linear =
          (progress == 0)
              ? 0.0f
              : (midi_fade_start_ * static_cast<float>(remaining) /
                 static_cast<float>(progress));
      const auto volume = static_cast<Volume>(
          std::clamp(static_cast<int>(linear * kMaxVolume + 0.5f), 0,
                     static_cast<int>(kMaxVolume)));
      volume_ = volume;
      ApplyMidiVolume();
    }
    StopIfFadeComplete();
  }
}

BgmSnapshot BgmController::Snapshot() const {
  BgmSnapshot snapshot;
  snapshot.mode = mode_;
  snapshot.state = state_;
  snapshot.title = title_;
  snapshot.tempo = tempo_;
  snapshot.gain_applied = gain_applied_;
  if (mode_ == BgmMode::Waveform) {
    snapshot.play_time = waveform_.PlayTime();
  } else if (mode_ == BgmMode::Midi && midi_loaded_) {
    snapshot.play_time = sequencer_.Realtime();
  }
  return snapshot;
}

bool BgmController::IsLoaded() const {
  return (mode_ != BgmMode::None);
}

void BgmController::ApplyMidiVolume() {
  for (std::uint8_t channel = 0; channel < kMidiChannelCount; channel++) {
    synth_.Output(static_cast<std::uint8_t>(0xb0 | channel), 0x07,
                  volume_);
  }
}

void BgmController::UpdateWaveformVolume() {
  if (!waveform_.IsLoaded()) {
    return;
  }
  float gain = 1.0f;
  if (gain_applied_) {
    if (const auto gain_factor = waveform_.GainFactor(); gain_factor) {
      gain = *gain_factor;
    }
  }
  waveform_.SetVolume(LinearVolume(volume_) * gain);
}

void BgmController::StopIfFadeComplete() {
  if (midi_fade_remaining_ == std::chrono::milliseconds::zero() &&
      midi_fade_duration_ > std::chrono::milliseconds::zero()) {
    midi_fade_duration_ = {};
    Stop();
  }
}

} // namespace audio::bgm

