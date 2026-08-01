/// Application-level BGM orchestration behind a single active Track.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

struct ma_engine;

#include "track.h"

#include "audio/core/audio_types.h"
#include "audio/midi/midi_parser.h"

namespace audio::midi {
class MidiSequencer;
class MidiSynth;
} // namespace audio::midi

namespace audio::bgm {

class BgmController {
public:
  BgmController(ma_engine &engine, midi::MidiSequencer &sequencer,
                midi::MidiSynth &synth);

  AudioResult LoadMidi(midi::SequenceData sequence);
  AudioResult LoadWaveform(std::string_view path);
  void ClearWaveform();

  void Play();
  void Stop();
  void Pause();
  void Resume();
  void FadeOut(float volume_start, std::chrono::milliseconds duration);

  void SetVolume(Volume volume);
  void SetTempo(std::int8_t tempo);
  void SetGainApplied(bool enabled);

  void Tick(std::chrono::milliseconds delta);

  [[nodiscard]] BgmSnapshot Snapshot() const;
  [[nodiscard]] bool IsLoaded() const;

private:
  void ApplyTrackSettings(Track &track);

  ma_engine &engine_;
  midi::MidiSequencer &sequencer_;
  midi::MidiSynth &synth_;
  std::unique_ptr<Track> midi_;
  std::unique_ptr<Track> waveform_;
  Track *active_ = nullptr;
  PlaybackState state_ = PlaybackState::Idle;
  std::atomic<bool> playing_ = false;
  bool gain_applied_ = true;
  Volume volume_ = kMaxVolume;
  std::int8_t tempo_ = 0;
};

} // namespace audio::bgm
