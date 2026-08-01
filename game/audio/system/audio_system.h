/// Composition root for the application-owned audio system.

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>

#include "audio/bgm/bgm_controller.h"
#include "audio/backend/audio_engine.h"
#include "audio/core/audio_types.h"
#include "audio/midi/midi_parser.h"
#include "audio/midi/midi_sequencer.h"
#include "audio/midi/midi_synth.h"
#include "audio/sfx/sfx_bank.h"
#include "audio/stream/waveform_player.h"

struct SDL_AudioSpec;

namespace audio {

class AudioSystem {
public:
  AudioSystem();
  ~AudioSystem();
  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;

  AudioResult Initialize(std::string_view data_path,
                         std::string_view preferred_soundfont = {});
  void Shutdown();

  [[nodiscard]] bool IsEnabled() const;
  [[nodiscard]] bool IsMidiAvailable() const;

  AudioResult LoadBgmMidi(midi::SequenceData sequence);
  AudioResult LoadBgmWaveform(std::string_view path);
  void ClearBgmWaveform();
  void PlayBgm();
  void StopBgm();
  void PauseBgm();
  void ResumeBgm();
  void FadeOutBgm(std::uint8_t speed);
  void SetBgmVolume(Volume volume);
  void SetSfxVolume(Volume volume);
  void SetBgmTempo(std::int8_t tempo);
  void SetBgmGainApplied(bool enabled);

  [[nodiscard]] BgmSnapshot BgmSnapshot() const;
  [[nodiscard]] midi::Visualization MidiVisualization() const;

  [[nodiscard]] std::size_t MidiDeviceCount() const;
  [[nodiscard]] std::optional<std::string>
  MidiDeviceNameAt(std::size_t index) const;
  [[nodiscard]] std::optional<midi::DeviceSource>
  MidiDeviceSourceAt(std::size_t index) const;
  [[nodiscard]] std::optional<std::string> MidiCurrentDeviceName() const;
  AudioResult SelectMidiDevice(std::size_t index);
  AudioResult ChangeMidiDevice(int direction);
  void SetMidiFixSysExBugs(bool enabled);

  AudioResult LoadSfx(std::uint8_t id, const SDL_AudioSpec &spec,
                      std::span<const std::uint8_t> pcm,
                      std::uint8_t max_instances);
  void PlaySfx(std::uint8_t id, float pan = 0.0f, bool loop = false);
  void StopSfx(std::uint8_t id);
  void StopAllSfx();

  void PauseAll();
  void ResumeAll();

private:
  void StartTimer();
  void StopTimer();

  backend::AudioEngine engine_;
  stream::WaveformPlayer waveform_;
  sfx::SfxBank sfx_;
  midi::MidiSequencer sequencer_;
  midi::MidiSynth synth_;
  bgm::BgmController bgm_;
  std::jthread timer_;

  bool initialized_ = false;
  bool midi_available_ = false;
  bool sfx_initialized_ = false;
  Volume bgm_volume_ = kMaxVolume;
  Volume sfx_volume_ = kMaxVolume;
};

} // namespace audio

