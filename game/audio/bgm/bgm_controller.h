/// Application-level BGM orchestration and playback state.

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include "audio/core/audio_types.h"
#include "audio/midi/midi_parser.h"

namespace audio::stream {
class WaveformPlayer;
}

namespace audio::midi {
class MidiSequencer;
class MidiSynth;
}

namespace audio::bgm {

class BgmController {
public:
  BgmController(stream::WaveformPlayer &waveform,
                midi::MidiSequencer &sequencer, midi::MidiSynth &synth);

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
  void ApplyMidiVolume();
  void UpdateWaveformVolume();
  void StopIfFadeComplete();

  stream::WaveformPlayer &waveform_;
  midi::MidiSequencer &sequencer_;
  midi::MidiSynth &synth_;

  BgmMode mode_ = BgmMode::None;
  PlaybackState state_ = PlaybackState::Idle;
  bool playing_ = false;
  bool midi_loaded_ = false;
  bool gain_applied_ = true;
  Volume volume_ = kMaxVolume;
  std::int8_t tempo_ = 0;
  std::string title_;

  std::chrono::milliseconds midi_fade_duration_{};
  std::chrono::milliseconds midi_fade_remaining_{};
  float midi_fade_start_ = 0.0f;
};

} // namespace audio::bgm

