/// Application-level BGM orchestration behind a single active Track.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

struct ma_engine;

#include "track.h"

#include "audio/bgm/midi/midi_parser.h"
#include "audio/core/audio_types.h"

namespace audio::bgm {

class MidiSequencer;
class MidiSynth;

class BgmController {
public:
  BgmController(ma_engine &engine, MidiSequencer &sequencer, MidiSynth &synth);

  AudioResult LoadMidi(SequenceData sequence);
  AudioResult LoadWaveform(std::string_view path);
  void ClearWaveform();

  void Play();
  void Stop();
  void Pause();
  void Resume();
  void FadeOut(float volume_start, std::chrono::milliseconds duration);

  void SetVolume(Volume volume);
  void SetTempo(std::int8_t tempo);

  void Tick(std::chrono::milliseconds delta);

  [[nodiscard]] BgmSnapshot Snapshot() const;
  [[nodiscard]] bool IsLoaded() const;

private:
  void ApplyTrackSettings(Track &track);

  ma_engine &engine_;
  MidiSequencer &sequencer_;
  MidiSynth &synth_;
  std::unique_ptr<Track> midi_;
  std::unique_ptr<Track> waveform_;
  Track *active_ = nullptr;
  PlaybackState state_ = PlaybackState::Idle;
  std::atomic<bool> playing_ = false;
  Volume volume_ = kMaxVolume;
  std::int8_t tempo_ = 0;
};

} // namespace audio::bgm
