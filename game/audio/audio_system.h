/// Composition root for the application-owned audio system.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include "bgm/midi/midi_sequencer.h"
#include "bgm/midi/midi_synth.h"
#include "core/audio_types.h"
#include "sfx.h"

namespace audio {

class AudioSystem {
public:
  AudioSystem();
  ~AudioSystem();
  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;
  AudioSystem(AudioSystem &&) = delete;
  AudioSystem &operator=(AudioSystem &&) = delete;

  AudioResult Initialize(std::string_view data_path,
                         std::string_view preferred_soundfont = {});
  void Shutdown();

  [[nodiscard]] bool IsEnabled() const;
  [[nodiscard]] bool IsMidiAvailable() const;

  bool EnableBgm(bool enabled, std::string_view soundfont = {});
  void SetVolumes(Volume bgm, Volume sfx);

  AudioResult LoadBgmMidi(bgm::SequenceData sequence);
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

  [[nodiscard]] BgmSnapshot BgmSnapshot() const;
  [[nodiscard]] bgm::Visualization MidiVisualization() const;

  [[nodiscard]] std::size_t MidiDeviceCount() const;
  [[nodiscard]] std::optional<std::string>
  MidiDeviceNameAt(std::size_t index) const;
  [[nodiscard]] std::optional<bgm::DeviceSource>
  MidiDeviceSourceAt(std::size_t index) const;
  [[nodiscard]] std::optional<std::string> MidiCurrentDeviceName() const;
  AudioResult SelectMidiDevice(std::size_t index);
  AudioResult ChangeMidiDevice(int direction);
  void SetMidiFixSysExBugs(bool enabled);

  AudioResult LoadSfx(std::uint8_t id, std::span<const std::uint8_t> wav,
                      std::uint8_t max_instances);
  void PlaySfx(SfxId id, int x = kSoundFieldCenterX, bool loop = false);
  void PlaySfx(std::uint8_t id, float pan = 0.0F, bool loop = false);
  void StopSfx(SfxId id);
  void StopAllSfx();

  void PauseAll();
  void ResumeAll();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace audio
