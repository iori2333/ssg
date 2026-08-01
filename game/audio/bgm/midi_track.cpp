#include "midi_track.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <utility>

#include "audio/core/audio_types.h"
#include "audio/midi/midi_sequencer.h"
#include "audio/midi/midi_synth.h"

namespace audio::bgm {
namespace {

constexpr std::uint8_t kMidiChannelCount = 16;
constexpr std::uint8_t kTempoDenominator = 128;

float LinearVolume(Volume volume) {
  return (static_cast<float>(volume) / static_cast<float>(kMaxVolume));
}

Volume LinearToVolume(float linear) {
  return static_cast<Volume>(
      std::clamp(static_cast<int>(linear * kMaxVolume + 0.5f), 0,
                 static_cast<int>(kMaxVolume)));
}

} // namespace

MidiTrack::MidiTrack(midi::MidiSequencer &sequencer, midi::MidiSynth &synth)
    : sequencer_(sequencer), synth_(synth) {}

void MidiTrack::Load(midi::SequenceData sequence) {
  title_ = sequence.title;
  sequencer_.Load(std::move(sequence));
  sequencer_.Rewind();
  fade_duration_ms_.store(0);
  fade_remaining_ms_.store(0);
  loaded_.store(true);
  playing_.store(false);
}

void MidiTrack::Clear() {
  loaded_.store(false);
  playing_.store(false);
  title_.clear();
  fade_duration_ms_.store(0);
  fade_remaining_ms_.store(0);
}

void MidiTrack::Play() {
  if (!loaded_.load()) {
    return;
  }
  synth_.Resume();
  sequencer_.Rewind();
  synth_.Panic();
  ApplyVolume();
  playing_.store(true);
}

void MidiTrack::Stop() {
  if (loaded_.load()) {
    synth_.Resume();
    synth_.Panic();
  }
  playing_.store(false);
  fade_duration_ms_.store(0);
  fade_remaining_ms_.store(0);
}

void MidiTrack::Pause() {
  if (!playing_.exchange(false)) {
    return;
  }
  synth_.Pause();
}

void MidiTrack::Resume() {
  if (playing_.load() || !loaded_.load()) {
    return;
  }
  synth_.Resume();
  playing_.store(true);
}

void MidiTrack::FadeOut(float volume_start,
                        std::chrono::milliseconds duration) {
  if (!loaded_.load()) {
    return;
  }
  fade_start_.store(volume_start);
  fade_duration_ms_.store(duration.count());
  fade_remaining_ms_.store(duration.count());
}

void MidiTrack::SetVolume(Volume volume) {
  volume_.store(std::min(volume, kMaxVolume));
  ApplyVolume();
}

void MidiTrack::SetTempo(std::int8_t tempo) {
  const auto clamped = std::clamp(tempo, std::int8_t{-100}, std::int8_t{100});
  const auto numerator = static_cast<std::uint8_t>(kTempoDenominator + clamped);
  sequencer_.SetTempo(numerator, kTempoDenominator);
}

void MidiTrack::SetGainApplied(bool) {}

void MidiTrack::Tick(std::chrono::milliseconds delta) {
  if (!playing_.load() || !loaded_.load()) {
    return;
  }

  sequencer_.Tick(delta, synth_, true);

  auto remaining = fade_remaining_ms_.load();
  if (remaining > 0) {
    remaining = std::max(remaining - delta.count(), std::int64_t{0});
    fade_remaining_ms_.store(remaining);
    const auto duration = fade_duration_ms_.load();
    if (duration > 0) {
      const auto linear = fade_start_.load() * static_cast<float>(remaining) /
                          static_cast<float>(duration);
      SetVolume(LinearToVolume(linear));
    }
  }

  if ((fade_remaining_ms_.load() == 0) && (fade_duration_ms_.load() > 0)) {
    fade_duration_ms_.store(0);
    Stop();
  }
}

void MidiTrack::TickBackground(std::chrono::milliseconds delta) {
  if (!loaded_.load() || playing_.load()) {
    return;
  }
  sequencer_.Tick(delta, synth_, false);
}

bool MidiTrack::IsLoaded() const { return loaded_.load(); }

bool MidiTrack::IsPlaying() const { return playing_.load(); }

BgmMode MidiTrack::Mode() const { return BgmMode::Midi; }

std::string_view MidiTrack::Title() const { return title_; }

std::chrono::milliseconds MidiTrack::PlayTime() const {
  return sequencer_.Realtime();
}

float MidiTrack::FadeVolumeLinear() const {
  const auto duration = fade_duration_ms_.load();
  if (duration <= 0) {
    return LinearVolume(volume_.load());
  }
  const auto remaining = fade_remaining_ms_.load();
  return fade_start_.load() * static_cast<float>(remaining) /
         static_cast<float>(duration);
}

void MidiTrack::ApplyVolume() {
  const auto volume = volume_.load();
  for (std::uint8_t channel = 0; channel < kMidiChannelCount; channel++) {
    synth_.Output(static_cast<std::uint8_t>(0xb0 | channel), 0x07, volume);
  }
}

} // namespace audio::bgm
