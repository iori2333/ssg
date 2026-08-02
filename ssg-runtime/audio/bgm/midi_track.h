/// MIDI BGM track backed by the shared sequencer and FluidSynth synth.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include "midi/midi_parser.h"
#include "track.h"

namespace audio::bgm {

class MidiSequencer;
class MidiSynth;

class MidiTrack final : public Track {
public:
  MidiTrack(MidiSequencer &sequencer, MidiSynth &synth);

  void Load(SequenceData sequence);
  void Clear();

  void Play() override;
  void Stop() override;
  void Pause() override;
  void Resume() override;
  void FadeOut(float volume_start, std::chrono::milliseconds duration) override;
  void SetVolume(Volume volume) override;
  void SetTempo(int tempo) override;
  void Tick(std::chrono::milliseconds delta) override;
  void TickBackground(std::chrono::milliseconds delta) override;

  [[nodiscard]] bool IsLoaded() const override;
  [[nodiscard]] bool IsPlaying() const override;
  [[nodiscard]] BgmMode Mode() const override;
  [[nodiscard]] std::chrono::milliseconds PlayTime() const override;
  [[nodiscard]] float FadeVolumeLinear() const override;

private:
  void ApplyVolume();

  MidiSequencer &sequencer_;
  MidiSynth &synth_;
  std::atomic<bool> loaded_ = false;
  std::atomic<bool> playing_ = false;
  std::atomic<Volume> volume_ = kMaxVolume;
  std::atomic<float> fade_start_ = 0.0F;
  std::atomic<std::int64_t> fade_duration_ms_ = 0;
  std::atomic<std::int64_t> fade_remaining_ms_ = 0;
};

} // namespace audio::bgm
