/// FluidSynth-backed MIDI output device manager.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "audio/core/audio_types.h"
#include "midi_sequencer.h"

namespace audio::midi {

enum class DeviceSource : std::uint8_t {
  System,
  Environment,
  Local,
};

class MidiSynth : public MidiSink {
public:
  MidiSynth();
  ~MidiSynth();
  MidiSynth(const MidiSynth &) = delete;
  MidiSynth &operator=(const MidiSynth &) = delete;

  AudioResult Initialize(std::string_view data_path,
                         std::string_view preferred_soundfont);
  void Shutdown();

  [[nodiscard]] std::size_t DeviceCount() const;
  [[nodiscard]] std::optional<std::string> DeviceName(std::size_t index) const;
  [[nodiscard]] std::optional<DeviceSource>
  DeviceSourceAt(std::size_t index) const;
  [[nodiscard]] std::optional<std::string> CurrentDeviceName() const;
  [[nodiscard]] bool IsInitialized() const;

  AudioResult SelectDevice(std::size_t index);
  AudioResult ChangeDevice(int direction);

  void Output(std::uint8_t status, std::uint8_t a,
              std::uint8_t b) override;
  void OutputSysEx(std::span<const std::uint8_t> message) override;
  void Panic();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace audio::midi

