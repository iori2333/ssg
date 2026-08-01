#include "midi_sequencer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "util/byte_io.h"

namespace audio::midi {
namespace {

using namespace std::chrono_literals;

constexpr std::uint8_t kChannelCount = 16;
constexpr std::uint8_t kNoteCount = 128;
constexpr std::uint32_t kVlqError = std::numeric_limits<std::uint32_t>::max();

enum class EventKind : std::uint8_t {
  NoteOff = 0x80,
  NoteOn = 0x90,
  NoteAftertouch = 0xa0,
  Controller = 0xb0,
  ProgramChange = 0xc0,
  ChannelAftertouch = 0xd0,
  PitchBend = 0xe0,
  SysEx = 0xf0,
  Meta = 0xff,
  First = NoteOff,
};

struct MidiEvent {
  EventKind kind;
  std::uint8_t status;
  std::uint8_t meta;
  std::span<const std::uint8_t> extra_data;

  [[nodiscard]] std::uint8_t Channel() const {
    return (status & 0x0f);
  }

  [[nodiscard]] bool IsNoteOff() const {
    return ((kind == EventKind::NoteOff) ||
            ((kind == EventKind::NoteOn) && (extra_data[1] == 0)));
  }
};

struct MidiTempo {
  std::chrono::nanoseconds qn_duration = 1s;
  std::uint16_t ppqn = 480;

  [[nodiscard]] std::chrono::nanoseconds
  RealtimeFromDelta(std::int32_t delta) const {
    const auto qn_times_delta = (qn_duration * delta);
    return ((delta > 0)
                ? ((qn_times_delta + std::chrono::nanoseconds(ppqn / 2)) /
                   ppqn)
                : ((qn_times_delta - std::chrono::nanoseconds(ppqn / 2)) /
                   ppqn));
  }

  [[nodiscard]] std::int32_t
  DeltaFromRealtime(std::chrono::nanoseconds realtime) const {
    return ((realtime * ppqn) / qn_duration);
  }
};

class MidiTrackIterator {
public:
  MidiTrackIterator() = default;
  explicit MidiTrackIterator(std::span<const std::uint8_t> data)
      : cursor_(data), initialized_(true) {}

  [[nodiscard]] explicit operator bool() const { return initialized_; }

  std::uint32_t ConsumeVlq() {
    std::uint32_t ret = 0;
    for (std::size_t i = 0; i < 4; i++) {
      const auto byte = cursor_.Read<std::uint8_t>();
      if (!byte) {
        return kVlqError;
      }
      ret = ((ret << 7) | (*byte & 0x7f));
      if ((*byte & 0x80) == 0) {
        return ret;
      }
    }
    return kVlqError;
  }

  std::optional<MidiEvent> ConsumeEvent() {
    const auto status_byte = cursor_.Peek<std::uint8_t>();
    if (!status_byte) {
      return std::nullopt;
    }
    if (*status_byte >= std::to_underlying(EventKind::First)) {
      status_ = *cursor_.Read<std::uint8_t>();
    }
    if (status_ < std::to_underlying(EventKind::First)) {
      return std::nullopt;
    }

    const auto kind = ((status_ > std::to_underlying(EventKind::SysEx))
                           ? EventKind::Meta
                           : static_cast<EventKind>(status_ & 0xf0));
    std::optional<std::span<const std::uint8_t>> extra_data;
    std::uint8_t meta = 0;
    switch (kind) {
    case EventKind::NoteOff:
    case EventKind::NoteOn:
    case EventKind::NoteAftertouch:
    case EventKind::Controller:
    case EventKind::PitchBend:
      extra_data = cursor_.ReadBytes(2);
      break;
    case EventKind::ProgramChange:
    case EventKind::ChannelAftertouch:
      extra_data = cursor_.ReadBytes(1);
      break;
    case EventKind::SysEx: {
      const auto length = ConsumeVlq();
      extra_data = (length == kVlqError)
                       ? std::nullopt
                       : cursor_.ReadBytes(static_cast<std::size_t>(length));
      break;
    }
    case EventKind::Meta: {
      const auto meta_byte = cursor_.Read<std::uint8_t>();
      const auto length = ConsumeVlq();
      if (!meta_byte || length == kVlqError) {
        return std::nullopt;
      }
      meta = *meta_byte;
      extra_data =
          cursor_.ReadBytes(static_cast<std::size_t>(length));
      break;
    }
    default:
      return std::nullopt;
    }

    if (!extra_data) {
      return std::nullopt;
    }
    return MidiEvent{kind, status_, meta, *extra_data};
  }

private:
  util::ByteReader cursor_;
  std::uint8_t status_ = 0;
  bool initialized_ = false;
};

struct MidiTrack {
  std::span<const std::uint8_t> data;
  MidiTrackIterator it;
  std::int64_t next_pulse = 0;
  std::chrono::nanoseconds next_time = 0ns;
  MidiTrackIterator loop_it;
  std::int64_t loop_pulse = 0;
  std::int64_t prev_pulse = 0;
  std::uint32_t next_delta = 0;
  bool play = true;

  void ConsumeDelta(const MidiTempo &tempo, const Loop &loop) {
    prev_pulse = next_pulse;
    next_delta = it.ConsumeVlq();
    if (next_delta == kVlqError) {
      play = false;
      return;
    }
    next_pulse += next_delta;

    if (loop.enabled) {
      if (!loop_it) {
        if (next_pulse >= loop.end) {
          play = false;
          return;
        }
        if (next_pulse >= loop.start) {
          loop_it = it;
          loop_pulse = next_pulse;
        }
      } else if (next_pulse > loop.end) {
        next_delta = ((loop.end - prev_pulse) +
                      (loop_pulse - loop.start));
        it = loop_it;
        next_pulse = loop_pulse;
        next_time += tempo.RealtimeFromDelta(next_delta);
        return;
      }
    }
    next_time += tempo.RealtimeFromDelta(next_delta);
  }
};

void SendEvent(const MidiEvent &event, MidiSink &sink, bool fix_sysex) {
  switch (event.kind) {
  case EventKind::SysEx: {
    std::vector<std::uint8_t> message(event.extra_data.size() + 1);
    message[0] = 0xf0;
    std::ranges::copy(event.extra_data, message.begin() + 1);

    if (fix_sysex && message.size() >= 10) {
      static constexpr std::uint8_t kSc88ReverbMacro[] = {
          0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x30,
      };
      if (std::equal(std::begin(kSc88ReverbMacro),
                     std::end(kSc88ReverbMacro),
                     event.extra_data.begin())) {
        const auto fix = std::min<std::uint8_t>(message[8], 7);
        if (fix != message[8]) {
          message[8] = fix;
          const std::uint8_t payload =
              (message[5] + message[6] + message[7] + message[8]);
          message[9] = (0x80 - (payload % 0x80));
        }
      }
    }
    sink.OutputSysEx(message);
    break;
  }
  case EventKind::Controller:
  case EventKind::NoteOn:
  case EventKind::NoteAftertouch:
  case EventKind::NoteOff:
  case EventKind::PitchBend:
    sink.Output(event.status, event.extra_data[0], event.extra_data[1]);
    break;
  case EventKind::ProgramChange:
  case EventKind::ChannelAftertouch:
    sink.Output(event.status, event.extra_data[0], 0);
    break;
  case EventKind::Meta:
    break;
  }
}

} // namespace

struct MidiSequencer::Impl {
  mutable std::mutex mutex;
  SequenceData sequence;
  std::vector<MidiTrack> tracks;
  MidiTempo tempo;
  Loop loop;
  std::uint8_t tempo_numerator = 1;
  std::uint8_t tempo_denominator = 1;
  bool fix_sysex = false;
  bool finished = false;
  Visualization visualization;

  void Rewind() {
    visualization.play_time = {};
    for (auto &track : tracks) {
      track.it = MidiTrackIterator{track.data};
      track.play = true;
      track.next_pulse = 0;
      track.loop_it = {};
      track.loop_pulse = 0;
      track.next_delta = 0;
      track.prev_pulse = 0;
      track.ConsumeDelta(tempo, loop);
    }
    finished = false;
  }

  void Process(MidiTrack &track, const MidiEvent &event) {
    if (event.kind == EventKind::Meta) {
      if (event.meta == 0x2f) {
        if (track.loop_it) {
          track.next_delta =
              ((loop.end - track.next_pulse) +
               (track.loop_pulse - loop.start));
          track.it = track.loop_it;
          track.next_pulse = track.loop_pulse;
          track.next_time +=
              tempo.RealtimeFromDelta(track.next_delta);
        } else {
          track.play = false;
        }
        return;
      }
      if (event.meta == 0x51) {
        std::uint32_t tempo_new = 0;
        for (const auto byte : event.extra_data) {
          tempo_new = ((tempo_new << 8) + byte);
        }
        tempo.qn_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::microseconds{tempo_new});
        for (auto &other : tracks) {
          const auto other_unlooped_next_pulse =
              (other.prev_pulse + other.next_delta);
          const auto delta =
              ((track.next_pulse >= other.prev_pulse)
                   ? (other_unlooped_next_pulse - track.next_pulse)
                   : (other.next_pulse - track.next_pulse));
          other.next_time =
              (track.next_time + tempo.RealtimeFromDelta(delta));
        }
      }
      track.ConsumeDelta(tempo, loop);
      return;
    }

    switch (event.kind) {
    case EventKind::Controller:
      switch (event.extra_data[0]) {
      case 0x07:
        visualization.volume[event.Channel()] = event.extra_data[1];
        break;
      case 0x0a:
        visualization.pan[event.Channel()] = event.extra_data[1];
        break;
      case 0x0b:
        visualization.expression[event.Channel()] = event.extra_data[1];
        break;
      default:
        break;
      }
      break;
    case EventKind::NoteOff:
      visualization.notes[event.Channel()][event.extra_data[0]] = 0;
      break;
    case EventKind::NoteOn:
    case EventKind::NoteAftertouch: {
      const auto channel = event.Channel();
      const auto note = event.extra_data[0];
      const auto velocity = event.extra_data[1];
      if (visualization.play[channel][note] < velocity) {
        visualization.play[channel][note] = velocity;
        visualization.levels[channel][note] = velocity;
      }
      visualization.notes[channel][note] = velocity;
      if (visualization.notes[channel][note]) {
        visualization.note_highlights[channel][note] = 5;
      }
      break;
    }
    default:
      break;
    }
    track.ConsumeDelta(tempo, loop);
  }
};

MidiSequencer::MidiSequencer() : impl_(std::make_unique<Impl>()) {}
MidiSequencer::~MidiSequencer() = default;

void MidiSequencer::Load(SequenceData sequence) {
  std::scoped_lock lock(impl_->mutex);
  impl_->sequence = std::move(sequence);
  impl_->tracks.clear();
  impl_->tracks.reserve(impl_->sequence.tracks.size());
  for (std::size_t i = 0; i < impl_->sequence.tracks.size(); i++) {
    impl_->tracks.push_back(
        MidiTrack{.data = impl_->sequence.Track(i)});
  }
  impl_->tempo.ppqn = impl_->sequence.ppqn;
  impl_->tempo.qn_duration = 1s;
  impl_->visualization = {};
  impl_->Rewind();
}

void MidiSequencer::Clear() {
  std::scoped_lock lock(impl_->mutex);
  impl_->sequence = {};
  impl_->tracks.clear();
  impl_->visualization = {};
  impl_->finished = false;
}

void MidiSequencer::SetLoop(const Loop &loop) {
  std::scoped_lock lock(impl_->mutex);
  impl_->loop = loop;
}

void MidiSequencer::SetTempo(std::uint8_t numerator,
                             std::uint8_t denominator) {
  std::scoped_lock lock(impl_->mutex);
  impl_->tempo_numerator = numerator;
  impl_->tempo_denominator = denominator;
}

void MidiSequencer::SetFixSysExBugs(bool enabled) {
  std::scoped_lock lock(impl_->mutex);
  impl_->fix_sysex = enabled;
}

void MidiSequencer::Rewind() {
  std::scoped_lock lock(impl_->mutex);
  impl_->Rewind();
}

void MidiSequencer::Tick(std::chrono::nanoseconds delta, MidiSink &sink,
                         bool output_enabled) {
  std::scoped_lock lock(impl_->mutex);
  if (impl_->sequence.tracks.empty()) {
    return;
  }

  const auto interval =
      ((delta * impl_->tempo_numerator) / impl_->tempo_denominator);
  auto &time = impl_->visualization.play_time;
  std::int64_t pulse_sync = 0;

  for (auto &track : impl_->tracks) {
    if (track.play) {
      track.next_time -= interval;
    }
  }
  time.realtime += std::chrono::round<decltype(time.realtime)>(delta);
  time.realtime_since_last_event += interval;

  for (auto &track : impl_->tracks) {
    while (track.play && (track.next_time <= 0ns)) {
      const auto event = track.it.ConsumeEvent();
      if (!event) {
        track.play = false;
        continue;
      }

      if (impl_->loop.enabled &&
          (track.next_pulse == impl_->loop.end) && !event->IsNoteOff()) {
        track.ConsumeDelta(impl_->tempo, impl_->loop);
        continue;
      }

      if (track.next_pulse > pulse_sync) {
        pulse_sync = track.next_pulse;
        time.realtime_since_last_event = -track.next_time;
      }
      impl_->Process(track, *event);
      if (output_enabled) {
        SendEvent(*event, sink, impl_->fix_sysex);
      }
    }
  }

  const bool still_playing =
      std::ranges::any_of(impl_->tracks, [](const auto &track) {
        return track.play;
      });
  if (pulse_sync != 0) {
    time.pulse_of_last_event_processed = pulse_sync;
  }

  if (!still_playing && !impl_->loop.enabled) {
    time.pulse_interpolated = time.pulse_of_last_event_processed;
    impl_->Rewind();
    return;
  }

  if (time.realtime >= 0s) {
    time.pulse_interpolated =
        (time.pulse_of_last_event_processed +
         impl_->tempo.DeltaFromRealtime(time.realtime_since_last_event));
  }

  if (!still_playing) {
    impl_->Rewind();
  }
}

Visualization MidiSequencer::Snapshot() const {
  std::scoped_lock lock(impl_->mutex);
  auto snapshot = impl_->visualization;
  snapshot.loaded = !impl_->sequence.tracks.empty();

  for (auto channel = 0UZ; channel < kChannelCount; channel++) {
    for (auto note = 0UZ; note < kNoteCount; note++) {
      auto &play = impl_->visualization.play[channel][note];
      if (play != 0) {
        play -= ((play >> 3) + 1);
      }
      auto &level = impl_->visualization.levels[channel][note];
      if (level != 0) {
        const auto decay =
            (std::max)(static_cast<int>(level / 50), 1);
        level = ((level >= decay)
                     ? static_cast<std::uint8_t>(level - decay)
                     : 0);
      }
      auto &highlight =
          impl_->visualization.note_highlights[channel][note];
      if (highlight != 0) {
        highlight--;
      }
    }
  }
  return snapshot;
}

bool MidiSequencer::IsLoaded() const {
  std::scoped_lock lock(impl_->mutex);
  return !impl_->sequence.tracks.empty();
}

bool MidiSequencer::IsFinished() const {
  std::scoped_lock lock(impl_->mutex);
  return impl_->finished;
}

std::chrono::milliseconds MidiSequencer::Realtime() const {
  std::scoped_lock lock(impl_->mutex);
  return impl_->visualization.play_time.realtime;
}

} // namespace audio::midi
