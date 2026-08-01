/// Stateful MIDI sequencer with visualization snapshots.

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "midi_parser.h"

namespace audio::midi {

struct Loop {
  std::int64_t start = 0;
  std::int64_t end = 0;
  bool enabled = false;
};

struct PlayTime {
  std::int64_t pulse_of_last_event_processed = 0;
  std::int64_t pulse_interpolated = 0;
  std::chrono::duration<std::int32_t, std::milli> realtime{};
  std::chrono::nanoseconds realtime_since_last_event{};
};

struct Visualization {
  std::array<std::array<std::uint8_t, 128>, 16> play{};
  std::array<std::array<std::uint8_t, 128>, 16> levels{};
  std::array<std::array<std::uint8_t, 128>, 16> notes{};
  std::array<std::array<std::uint8_t, 128>, 16> note_highlights{};
  std::array<std::uint8_t, 16> pan{};
  std::array<std::uint8_t, 16> expression{};
  std::array<std::uint8_t, 16> volume{};
  PlayTime play_time{};
  bool loaded = false;
};

class MidiSink {
public:
  virtual ~MidiSink() = default;

  virtual void Output(std::uint8_t status, std::uint8_t a,
                      std::uint8_t b) = 0;
  virtual void OutputSysEx(std::span<const std::uint8_t> message) = 0;
};

class MidiSequencer {
public:
  MidiSequencer();
  ~MidiSequencer();
  MidiSequencer(const MidiSequencer &) = delete;
  MidiSequencer &operator=(const MidiSequencer &) = delete;
  MidiSequencer(MidiSequencer &&) = delete;
  MidiSequencer &operator=(MidiSequencer &&) = delete;

  void Load(SequenceData sequence);
  void Clear();

  void SetLoop(const Loop &loop);
  void SetTempo(std::uint8_t numerator, std::uint8_t denominator);
  void SetFixSysExBugs(bool enabled);

  void Rewind();
  void Tick(std::chrono::nanoseconds delta, MidiSink &sink,
            bool output_enabled);

  [[nodiscard]] Visualization Snapshot() const;
  [[nodiscard]] bool IsLoaded() const;
  [[nodiscard]] bool IsFinished() const;
  [[nodiscard]] std::chrono::milliseconds Realtime() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace audio::midi
