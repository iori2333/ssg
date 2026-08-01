/// MIDI BGM track backed by the shared sequencer and FluidSynth synth.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include "track.h"

#include "audio/midi/midi_parser.h"

namespace audio::midi {
class MidiSequencer;
class MidiSynth;
} // namespace audio::midi

namespace audio::bgm {

class MidiTrack final : public Track {
public:
  MidiTrack(midi::MidiSequencer &sequencer, midi::MidiSynth &synth);

  void Load(midi::SequenceData sequence);
  void Clear();

  void Play() override;
  void Stop() override;
  void Pause() override;
  void Resume() override;
  void FadeOut(float volume_start, std::chrono::milliseconds duration) override;
  void SetVolume(Volume volume) override;
  void SetTempo(std::int8_t tempo) override;
  void SetGainApplied(bool enabled) override;
  void Tick(std::chrono::milliseconds delta) override;
  void TickBackground(std::chrono::milliseconds delta) override;

  [[nodiscard]] bool IsLoaded() const override;
  [[nodiscard]] bool IsPlaying() const override;
  [[nodiscard]] BgmMode Mode() const override;
  [[nodiscard]] std::string_view Title() const override;
  [[nodiscard]] std::chrono::milliseconds PlayTime() const override;
  [[nodiscard]] float FadeVolumeLinear() const override;

private:
  void ApplyVolume();

  midi::MidiSequencer &sequencer_;
  midi::MidiSynth &synth_;
  std::string title_;
  std::atomic<bool> loaded_ = false;
  std::atomic<bool> playing_ = false;
  std::atomic<Volume> volume_ = kMaxVolume;
  std::atomic<float> fade_start_ = 0.0f;
  std::atomic<std::int64_t> fade_duration_ms_ = 0;
  std::atomic<std::int64_t> fade_remaining_ms_ = 0;
};

} // namespace audio::bgm
