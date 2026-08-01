///
/// MIDI management functions
///

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.
#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <thread>

#include <malloc.h>

#include "midi.h"
#include "midi_backend.h"
#include "volume.h"

#include "util/byte_io.h"
#include "util/endian.h"
#include "util/enum_flags.h"

using namespace std::chrono_literals;

// MIDI protocol
// -------------

// Valid values for the upper nibble of the MIDI status byte.
enum class MidiEventKind : uint8_t {
  NoteOff = 0x80,
  NoteOn = 0x90,
  NoteAftertouch = 0xA0,
  Controller = 0xB0,
  ProgramChange = 0xC0,
  ChannelAftertouch = 0xD0,
  PitchBend = 0xE0,
  SysEx = 0xF0,
  Meta = 0xFF,

  First = NoteOff,
};
// -------------

// MIDI protocol
// -------------

static constexpr uint8_t kMidiChannelCount = 16;
// -------------

// Standard MIDI File format
// -------------------------
#pragma pack(push, 1)

struct SmfFileHeader {
  util::BigEndian<uint32_t> magic;
  util::BigEndian<uint32_t> size;
};

struct SmfMainHeader {
  util::BigEndian<uint16_t> format;
  util::BigEndian<uint16_t> track;
  util::BigEndian<uint16_t> timebase;
};

struct SmfTrackHeader {
  util::BigEndian<uint32_t> magic;
  util::BigEndian<uint32_t> size;
};

#pragma pack(pop)
// -------------------------

// MIDI device structure
struct MidiDevice {
  // The following must not be modified or referenced externally
  MidiRealtime fade_progress;
  MidiRealtime fade_duration;
  AudioVolume fade_start_volume;
  AudioVolume fade_end_volume;

  // *Always* between 0 and 127.
  AudioVolume fade_volume;

  MidiBackendState state; // Current state

  AudioVolume VolumeFor(decltype(kMidiChannelCount) ch) const;
  void ApplyVolume() const;
  void FadeIO(MidiRealtime delta);
};

struct MidiTempo {
  MidiRealtime qn_duration;
  uint16_t ppqn;

  MidiRealtime RealtimeFromDelta(int32_t delta) const {
    // Let's round to average out any truncated nanoseconds...
    const auto qn_times_delta = (qn_duration * delta);
    return ((delta > 0) ? ((qn_times_delta + MidiRealtime(ppqn / 2)) / ppqn)
                        : ((qn_times_delta - MidiRealtime(ppqn / 2)) / ppqn));
  }
  int32_t DeltaFromRealtime(MidiRealtime realtime) const {
    return ((realtime * ppqn) / qn_duration);
  }
};

struct MidiEvent {
  // An exact `enum class` value for easy `switch`ing.
  MidiEventKind kind;

  // Raw MIDI status byte, including the channel if <0xF0.
  uint8_t status;

  // Meta event code; only valid if ([status] == MidiEventKind::Meta).
  uint8_t meta;

  // Extra data after the status byte. *Not* a complete raw MIDI message!
  std::span<const uint8_t> extra_data;

  uint8_t Channel() const {
    assert(kind < MidiEventKind::SysEx);
    return (status & 0xf);
  }

  bool IsNoteOff() const {
    return ((kind == MidiEventKind::NoteOff) ||
            ((kind == MidiEventKind::NoteOn) && (extra_data[1] == 0)));
  }

  // Converts the event to a raw MIDI message and sends that to the output
  // device.
  void Send() const;
};

class MidiTrackIterator {
  util::ByteReader cursor;
  uint8_t status = 0x00;
  bool initialized = false;

public:
  MidiTrackIterator() = default;
  MidiTrackIterator(const MidiTrackIterator &other) = default;
  MidiTrackIterator(const std::span<const uint8_t> data)
      : cursor(data), status(0x00), initialized(true) {}

  explicit operator bool() const { return initialized; }

  // Reads a MIDI variable-length quantity and advances [work] accordingly.
  // Used for both delta times and multi-byte event lengths. Returns -1 if
  // the end of the track was reached in the middle of the parsed value.
  uint32_t ConsumeVLQ();

  // Reads the next MIDI event and advances [work] according to its size.
  // Returns std::nullopt if the end of the sequence has been reached.
  std::optional<MidiEvent> ConsumeEvent();
};

struct MidiTrack {
  std::span<const uint8_t> data;
  MidiTrackIterator it;
  MidiPulse next_pulse = 0;

  // Time until the next MIDI event. The MIDI events add their delta time
  // converted into seconds according to the current tempo, and the play
  // callback subtracts its interval. If ≤0, the next event is processed.
  MidiRealtime next_time = 0s;

  // [it] and [next_pulse] for the first event after the loop start point.
  MidiTrackIterator loop_it;
  MidiPulse loop_pulse = 0;

  // Only necessary for resynchronizing [next_time] after a tempo change on
  // another track.
  MidiPulse prev_pulse = 0;
  uint32_t next_delta = 0;

  bool play = true;

  // Reads a MIDI delta time from the iterator and updates [next_time] and
  // [next_pulse] accordingly.
  void ConsumeDelta(const MidiTempo &tempo, const MidiLoop &loop);
};

struct MidiSequence {
  std::vector<uint8_t> smf;
  std::unique_ptr<MidiTrack[]> track_buf = nullptr;
  std::span<MidiTrack> tracks;
  MidiTempo tempo = {.qn_duration = 1s /* 60 BPM */};
  MidiLoop loop;

  // Applies the event to the sequence state and consumes the following delta
  // time on the given track.
  void Process(MidiTrack &track, const MidiEvent &event);

  void Rewind();
};

namespace {

struct MidiState {
  MidiDevice device;
  MidiFlags flags = MidiFlags::None;
  MidiSequence sequence;
  std::recursive_mutex mutex;
  MidiVisualization visualization;
  AudioVolume volume = kMaxAudioVolume;
  uint8_t tempo_numerator = 1;
  uint8_t tempo_denominator = 1;
};

MidiState &State() {
  static MidiState state;
  return state;
}

} // namespace

MidiFlags MidiSetFlags(MidiFlags flags_new) {
  using F = MidiFlags;

  const auto new_sysex = !!(flags_new & F::FixSysExBugs);
  bool restart = false;
  {
    const std::scoped_lock lock(State().mutex);
    if (!!(State().flags & F::FixSysExBugs) == new_sysex) {
      return State().flags;
    }
    SetEnumFlag(State().flags, F::FixSysExBugs, new_sysex);
    restart = (State().device.state == MidiBackendState::Playing);
  }
  if (restart) {
    MidiStop();
    MidiPlay();
  }
  return State().flags;
}

void MidiPlay() {
  const std::scoped_lock lock(State().mutex);
  if (!MidiBackendDeviceName() || State().sequence.tracks.empty() ||
      (State().device.state == MidiBackendState::Playing)) {
    return;
  }

  State().device.fade_duration = 0s;
  State().device.fade_volume = kMaxAudioVolume;

  // Master Volume: F0 7F 7F 04 01 VolumeLowByte VolumeHighByte F7
  // Lower byte is apparently treated as 00 on the Roland SC-88ST Pro (per the
  // manual)
  //
  // On both the Microsoft GS Wavetable Synth and all Yamaha XG synths I
  // tested on, sending a MIDI Universal Realtime Master Volume message right
  // before the GM System On below has no effect because the latter also
  // resets the Master Volume. This might not necessarily be true on Roland
  // synths though, so let's better keep this relic from the original code.
  uint8_t msg[8] = {0xf0, 0x7f, 0x7f, 0x04, 0x01, 0x00, kMaxAudioVolume, 0xf7};
  MidiBackendOutput(msg);

  State().sequence.Rewind();
  for (auto &t : State().sequence.tracks) {
    t.next_time = 0s;
  }

  // GM SystemOn : F0H 7EH 7FH 09H 01H F7H
  // Followed by a 50 ms sleep, whose necessity is explained at
  //
  // 	https://github.com/nmlgc/ssg/issues/10#issuecomment-1938245315
  uint8_t msg_gm_system_on[6] = {0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7};
  MidiBackendOutput(msg_gm_system_on);
  std::this_thread::sleep_for(50ms); // Make sure to wait at least 50ms here!

  MidiBackendStartTimer();
  State().device.state = MidiBackendState::Playing;
}

void MidiStop() {
  MidiBackendStopTimer();
  const std::scoped_lock lock(State().mutex);
  if (State().device.state == MidiBackendState::Stopped) {
    return;
  }

  State().device.fade_duration = 0s;

  MidiBackendPanic();

  MidiResetTables();
  for (auto i = 0; i < kMidiChannelCount; i++) {
    MidiBackendOutput((0xb0 + i), 0x7b, 0x00); // All Notes Off
    MidiBackendOutput((0xb0 + i), 0x78, 0x00); // All Sound Off
  }

  State().device.state = MidiBackendState::Stopped;
}

void MidiPause() {
  MidiBackendStopTimer();
  const std::scoped_lock lock(State().mutex);
  if (State().device.state != MidiBackendState::Playing) {
    return;
  }
  State().device.state = MidiBackendState::Paused;

  // Set volume on all channels to 0 while leaving all notes playing.
  // Not perfect, as sustained notes will continue to be sampled and will
  // play at a different sample position once we resume, but it's better than
  // the alternative of cutting off any playing notes altogether.
  for (auto i = decltype(kMidiChannelCount){0}; i < kMidiChannelCount; i++) {
    MidiBackendOutput((0xb0 + i), 0x07, 0);
  }
}

void MidiResume() {
  const std::scoped_lock lock(State().mutex);
  if (State().device.state != MidiBackendState::Paused) {
    return;
  }
  State().device.ApplyVolume();
  MidiBackendStartTimer();
  State().device.state = MidiBackendState::Playing;
}

// Initialize various tables
void MidiResetTables() {
  const std::scoped_lock lock(State().mutex);
  for (auto i = 0; i < kMidiChannelCount; i++) {
    for (auto j = 0; j < 128; j++) {
      State().visualization.play[i][j] = 0;
      State().visualization.levels[i][j] = 0;
      State().visualization.notes[i][j] = 0;
      State().visualization.note_highlights[i][j] = 0;
    }

    State().visualization.pan[i] = 0x40;
    State().visualization.expression[i] = 0x7f;
    State().visualization.volume[i] = 0x64;
  }
}

AudioVolume MidiFadeVolume() {
  const std::scoped_lock lock(State().mutex);
  return State().device.fade_volume;
}

void MidiUpdateVolume() {
  const std::scoped_lock lock(State().mutex);
  State().device.ApplyVolume();
}

void MidiSetVolume(AudioVolume volume) {
  const std::scoped_lock lock(State().mutex);
  State().volume = std::min(volume, kMaxAudioVolume);
  MidiUpdateVolume();
}

void MidiFadeOut(AudioVolume volume_start, std::chrono::milliseconds duration) {
  const std::scoped_lock lock(State().mutex);
  State().device.fade_start_volume = volume_start;
  State().device.fade_end_volume = 0;
  State().device.fade_progress = 0s;
  State().device.fade_duration = duration;
}

bool MidiLoad(std::vector<uint8_t> buffer) {
  const std::scoped_lock lock(State().mutex);
  State().sequence = {};
  State().sequence.smf = std::move(buffer);

  util::ByteReader cursor{State().sequence.smf};
  const auto midhead = cursor.ReadObject<SmfFileHeader>();
  if (!midhead) {
    return false;
  }

  if (midhead->magic != 0x4D546864) { // "MThd"
    return false;
  }

  const auto midmain = cursor.ReadObject<SmfMainHeader>();
  if (!midmain) {
    return false;
  }

  State().sequence.tempo.ppqn = midmain->timebase;

  State().sequence.track_buf = std::unique_ptr<MidiTrack[]>(
      new (std::nothrow) MidiTrack[midmain->track]);
  if (!State().sequence.track_buf) {
    return false;
  }
  State().sequence.tracks = {State().sequence.track_buf.get(), midmain->track};

  for (auto &track : State().sequence.tracks) {
    const auto midtrack = cursor.ReadObject<SmfTrackHeader>();
    if (!midtrack) {
      return false;
    }

    const auto data = cursor.ReadBytes(midtrack->size);
    if (!data) {
      return false;
    }
    track.data = *data;
  }

  State().sequence.Rewind();
  return true;
}

void MidiSetLoop(const MidiLoop &loop) {
  const std::scoped_lock lock(State().mutex);
  State().sequence.loop = loop;
}

void MidiSetTempo(uint8_t numerator, uint8_t denominator) {
  const std::scoped_lock lock(State().mutex);
  State().tempo_numerator = numerator;
  State().tempo_denominator = denominator;
}

bool MidiIsLoaded() {
  const std::scoped_lock lock(State().mutex);
  return !State().sequence.tracks.empty();
}

MidiPlayTime MidiPlayTimePosition() {
  const std::scoped_lock lock(State().mutex);
  return State().visualization.play_time;
}

MidiVisualization MidiVisualizationState() {
  const std::scoped_lock lock(State().mutex);
  auto snapshot = State().visualization;
  snapshot.loaded = !State().sequence.tracks.empty();

  for (auto channel = 0UZ; channel < kMidiChannelCount; channel++) {
    for (auto note = 0UZ; note < 128; note++) {
      auto &play = State().visualization.play[channel][note];
      if (play != 0) {
        play -= ((play >> 3) + 1);
      }
      auto &level = State().visualization.levels[channel][note];
      if (level != 0) {
        level -= (std::max)((level / 50), 1);
      }
      auto &highlight = State().visualization.note_highlights[channel][note];
      if (highlight != 0) {
        highlight--;
      }
    }
  }
  return snapshot;
}

void MidiSequence::Rewind() {
  //  BgmSetTempo(0);

  State().visualization.play_time = {};
  for (auto &t : State().sequence.tracks) {
    // We do *not* reset [next_time] here. This preserves the overshot
    // amount of time when we rewind at the end of the track.
    t.it = {t.data};
    t.play = true;
    t.next_pulse = 0;
    t.loop_it = {};
    t.loop_pulse = 0;
    t.next_delta = 0;
    t.prev_pulse = 0;
    t.ConsumeDelta(tempo, loop);
  }
}

// Loops [track] from [cur_pulse] to the loop start point.
static void TrackLoop(MidiTrack &track, const MidiTempo &tempo,
                      const MidiLoop &loop, MidiPulse cur_pulse) {
  track.next_delta = ((loop.end - cur_pulse) + (track.loop_pulse - loop.start));
  track.it = track.loop_it;
  track.next_pulse = track.loop_pulse;
  track.next_time += tempo.RealtimeFromDelta(track.next_delta);
}

uint32_t MidiTrackIterator::ConsumeVLQ() {
  uint8_t temp = 0;
  uint32_t ret = 0;
  do {
    const auto maybe_temp = cursor.Read<uint8_t>();
    if (!maybe_temp) {
      return (std::numeric_limits<uint32_t>::max)();
    }
    temp = *maybe_temp;
    ret = ((ret << 7) | (temp & 0x7f));
  } while (temp & 0x80);
  return ret;
}

void MidiTrack::ConsumeDelta(const MidiTempo &tempo, const MidiLoop &loop) {
  prev_pulse = next_pulse;
  next_delta = it.ConsumeVLQ();
  if (next_delta == -1) {
    play = false;
    return;
  }
  next_pulse += next_delta;
  if (loop) {
    if (!loop_it) {
      if (next_pulse >= loop.end) {
        // The track won't play an event until after the loop, so we
        // can immediately shut it down.
        play = false;
      } else if (next_pulse >= loop.start) {
        // Set loop start point
        loop_it = it;
        loop_pulse = next_pulse;
      }
    } else if (next_pulse == loop.end) {
      // Handled in the timing loop.
    } else if (next_pulse > loop.end) {
      TrackLoop(*this, tempo, loop, prev_pulse);
      return;
    }
  }
  next_time += tempo.RealtimeFromDelta(next_delta);
}

std::optional<MidiEvent> MidiTrackIterator::ConsumeEvent() {
  const auto maybe_status = cursor.Peek<uint8_t>();
  if (!maybe_status) {
    return std::nullopt;
  }
  if (*maybe_status >= std::to_underlying(MidiEventKind::First)) {
    status = *cursor.Read<uint8_t>();
  }
  assert(status >= std::to_underlying(MidiEventKind::First));
  const auto kind = ((status > std::to_underlying(MidiEventKind::SysEx))
                         ? MidiEventKind::Meta
                         : static_cast<MidiEventKind>(status & 0xf0));

  std::optional<std::span<const uint8_t>> extra_data = std::nullopt;
  uint8_t meta = 0;
  switch (kind) {
  case MidiEventKind::NoteOff:
  case MidiEventKind::NoteOn:
  case MidiEventKind::NoteAftertouch:
  case MidiEventKind::Controller:
  case MidiEventKind::PitchBend:
    extra_data = cursor.ReadBytes(2);
    break;
  case MidiEventKind::ProgramChange:
  case MidiEventKind::ChannelAftertouch:
    extra_data = cursor.ReadBytes(1);
    break;
  case MidiEventKind::SysEx:
    extra_data = cursor.ReadBytes(ConsumeVLQ());
    break;
  case MidiEventKind::Meta:
    meta = cursor.Read<uint8_t>().value_or(0);
    extra_data = cursor.ReadBytes(ConsumeVLQ());
    break;
  default:
    assert(!"Unimplemented MIDI system message");
    break;
  }
  if (!extra_data) {
    return std::nullopt;
  }
  return MidiEvent{kind, status, meta, extra_data.value()};
}

AudioVolume MidiDevice::VolumeFor(decltype(kMidiChannelCount) ch) const {
  return ((State().visualization.volume[ch] * fade_volume * State().volume) /
          ((kMaxAudioVolume + 1) * (kMaxAudioVolume + 1)));
}

void MidiDevice::ApplyVolume() const {
  for (auto ch = decltype(kMidiChannelCount){0}; ch < kMidiChannelCount; ch++) {
    MidiBackendOutput((0xb0 + ch), 0x07, VolumeFor(ch));
  }
}

void MidiDevice::FadeIO(MidiRealtime delta) {
  if (fade_duration == 0s) {
    return;
  }
  fade_progress += delta;

  const auto fade_delta = (int{fade_end_volume} - fade_start_volume);
  const uint8_t new_volume =
      (fade_start_volume + ((fade_delta * fade_progress) / fade_duration));
  if (new_volume != fade_volume) {
    fade_volume = new_volume;

    ApplyVolume();

    if (fade_volume == fade_end_volume) {
      fade_duration = 0s;
      if (fade_volume == 0) {
        MidiStop();
      }
    }
  }
}

void MidiProcess(MidiRealtime delta) {
  const std::scoped_lock lock(State().mutex);
  if (!MidiIsLoaded()) {
    return;
  }

  const auto interval =
      ((delta * State().tempo_numerator) / State().tempo_denominator);
  auto &time = State().visualization.play_time;
  MidiPulse pulse_sync = 0;

  // Advance the timer of all tracks first. Must be done before we process
  // anything because tempo events on other tracks will need to resynchronize
  // the timer.
  for (auto &p : State().sequence.tracks) {
    if (p.play) {
      p.next_time -= interval;
    }
  }
  time.realtime += std::chrono::round<decltype(time.realtime)>(delta);
  time.realtime_since_last_event += interval;

  // Process all events.
  for (auto &p : State().sequence.tracks) {
    while (p.play && (p.next_time <= 0s)) {
      const auto maybe_event = p.it.ConsumeEvent();
      if (!maybe_event) {
        p.play = false;
        continue;
      }
      const auto &event = maybe_event.value();

      // We should have looped at this exact point, but doing that would
      // skip any Note Off messages that occur exactly at the loop end
      // point. Process only those and ignore everything else.
      if (State().sequence.loop &&
          (p.next_pulse == State().sequence.loop.end) && !event.IsNoteOff()) {
        p.ConsumeDelta(State().sequence.tempo, State().sequence.loop);
      } else {
        if (p.next_pulse > pulse_sync) {
          pulse_sync = p.next_pulse;
          time.realtime_since_last_event = -p.next_time;
        }
        State().sequence.Process(p, event);
        if (State().device.state == MidiBackendState::Playing) {
          event.Send();
        }
      }
    }
  }

  // Check if we're done. (We might have processed the final End of Track
  // event in the loop above.)
  const bool still_playing =
      std::ranges::any_of(State().sequence.tracks, &MidiTrack::play);

  if (pulse_sync != 0) {
    time.pulse_of_last_event_processed = pulse_sync;
  }

  // If the track doesn't loop, the timer should stop in place.
  if (!still_playing && (State().sequence.loop.end == -1)) {
    MidiStop();
    time.pulse_interpolated = time.pulse_of_last_event_processed;
    return;
  }

  if (time.realtime >= 0s) {
    time.pulse_interpolated = (time.pulse_of_last_event_processed +
                               State().sequence.tempo.DeltaFromRealtime(
                                   time.realtime_since_last_event));
  }

  State().device.FadeIO(delta);

  if (!still_playing) {
    State().sequence.Rewind();
  }
}

void MidiEvent::Send() const {
  switch (kind) {
  case MidiEventKind::SysEx: { // SysEx
    auto *msg = static_cast<uint8_t *>(malloc(extra_data.size() + 1));
    if (!msg) {
      break;
    }
    msg[0] = 0xf0;
    std::ranges::copy(extra_data, (msg + 1));

    /// Patch broken SysEx commands, if requested
    /// -----------------------------------------

    if (!!(State().flags & MidiFlags::FixSysExBugs)) {
      // The SC-88 and SC-88Pro manuals don't define how invalid Reverb
      // Macro messages are processed. The observable behavior does in
      // fact differ across synths:
      //
      // • A real-hardware SC-88Pro clamps such invalid messages to the
      //   valid range:
      //
      //   https://twitter.com/Romantique_Tp/status/1766895996645056902
      //
      // • The SC-8850 and Sound Canvas VA ignore these invalid messages
      //   just like any other invalid SysEx message, leaving the Reverb
      //   Macro at its previous value.
      //
      // Replicating the clamping here preserves the SC-88Pro response
      // for other Roland synths that don't clamp.
      static constexpr uint8_t kSc88ReverbMacro[] = {0x41, 0x10, 0x42, 0x12,
                                                     0x40, 0x01, 0x30};
      if (extra_data.size() >= (std::size(kSc88ReverbMacro) + 2) &&
          std::equal(std::begin(kSc88ReverbMacro), std::end(kSc88ReverbMacro),
                     extra_data.begin())) {
        const auto fix = std::min<uint8_t>(msg[8], 7);
        if (fix != msg[8]) {
          msg[8] = fix;

          // Fix SysEx checksum
          const uint8_t payload = (msg[5] + msg[6] + msg[7] + msg[8]);
          msg[9] = (0x80 - (payload % 0x80));
        }
      }
    }
    /// -----------------------------------------

    MidiBackendOutput(std::span{msg, (extra_data.size() + 1)});
    free(msg);
    break;
  }

  // 3 bytes: Control Change or Note On or Aftertouch or Note Off
  case MidiEventKind::Controller:
    if (extra_data[0] == 0x07) {
      MidiBackendOutput(status, 0x07, State().device.VolumeFor(Channel()));
    } else {
      MidiBackendOutput(status, extra_data[0], extra_data[1]);
    }
    break;

  case MidiEventKind::NoteOn:
  case MidiEventKind::NoteAftertouch:
  case MidiEventKind::NoteOff:
  case MidiEventKind::PitchBend:
    MidiBackendOutput(status, extra_data[0], extra_data[1]);
    break;

  // 2 bytes
  case MidiEventKind::ProgramChange:
  case MidiEventKind::ChannelAftertouch:
    MidiBackendOutput(status, extra_data[0]);
    break;

  case MidiEventKind::Meta:
    return;
  }
}

void MidiSequence::Process(MidiTrack &track, const MidiEvent &event) {
  switch (event.kind) {
  case MidiEventKind::Meta: // Meta events (only those without output are
                            // processed)
    switch (event.meta) {
    case 0x2f: // End of Track
      if (track.loop_it) {
        // Rewind to the first note after the loop
        TrackLoop(track, tempo, loop, track.next_pulse);
      } else {
        track.play = false;
      }
      return;

    case 0x51: { // Tempo
      uint32_t tempo_new = 0;
      for (const auto byte : event.extra_data) {
        tempo_new = ((tempo_new << 8) + byte);
      }
      tempo.qn_duration = std::chrono::duration_cast<MidiRealtime>(
          std::chrono::microseconds{tempo_new});

      // Recalculate the next tick on all tracks, preserving the amount
      // of time we've overshot when reaching this one.
      for (auto &other : tracks) {
        // Note that this is also correct in case [other] still has to
        // process multiple events before [track.next_pulse]. In this
        // case, all of these events *must* be processed now, which we
        // ensure by calculating the new [next_time] at the current
        // tempo.

        // This can be different than [next_pulse] if that track
        // already looped back.
        const auto other_unlooped_next_pulse =
            (other.prev_pulse + other.next_delta);
        const auto delta = ((track.next_pulse >= other.prev_pulse)
                                ? (other_unlooped_next_pulse - track.next_pulse)
                                : (other.next_pulse - track.next_pulse));
        other.next_time = (track.next_time + tempo.RealtimeFromDelta(delta));
      }
      break;
    }

    default:
      break;
    }

    // There is a mysterious line here
    break;

  case MidiEventKind::Controller: // Control Change
    switch (event.extra_data[0]) {
    case 0x07: // Volume
      State().visualization.volume[event.Channel()] = event.extra_data[1];
      break;
    case 0x0a: // Panpot
      State().visualization.pan[event.Channel()] = event.extra_data[1];
      break;
    case 0x0b: // Expression
      State().visualization.expression[event.Channel()] = event.extra_data[1];
      break;
    default:
      break;
    }
    break;

  case MidiEventKind::NoteOff: // Note Off
    State().visualization.notes[event.Channel()][event.extra_data[0]] = 0;
    break;

  case MidiEventKind::NoteOn:
  case MidiEventKind::NoteAftertouch: { // 3 bytes: Note On or Aftertouch
    const auto channel = event.Channel();
    const auto note = event.extra_data[0];
    const auto vel = event.extra_data[1];

    if (State().visualization.play[channel][note] < vel) {
      State().visualization.play[channel][note] = vel;
      State().visualization.levels[channel][note] = vel;
    }
    State().visualization.notes[channel][note] = vel;
    if (State().visualization.notes[channel][note]) {
      State().visualization.note_highlights[channel][note] = 5;
    }
    break;
  }

  default:
    break;
  }

  track.ConsumeDelta(tempo, loop);
}

std::string_view MidiTitle() {
  const std::scoped_lock lock(State().mutex);
  std::optional<MidiEvent> maybe_ev;

  const auto extra_data_as_string_view = [](const MidiEvent &ev) {
    const auto *str = reinterpret_cast<const char *>(ev.extra_data.data());
    return std::string_view{str, ev.extra_data.size()};
  };

  // For normal files; may display wrong data for malformed files
  for (const auto &track : State().sequence.tracks) {
    MidiTrackIterator it = {track.data};
    while ((it.ConsumeVLQ() != -1) && (maybe_ev = it.ConsumeEvent())) {
      const auto &ev = maybe_ev.value();
      // Sequence Name
      if ((ev.kind == MidiEventKind::Meta) && (ev.meta == 0x03)) {
        return extra_data_as_string_view(ev);
      }
    }
  }

  // For files where the title is stored in a different location
  for (const auto &track : State().sequence.tracks) {
    MidiTrackIterator it = {track.data};
    while ((it.ConsumeVLQ() != -1) && (maybe_ev = it.ConsumeEvent())) {
      const auto &ev = maybe_ev.value();
      // Text Event
      if ((ev.kind == MidiEventKind::Meta) && (ev.meta == 0x01)) {
        return extra_data_as_string_view(ev);
      }
    }
  }

  return {};
}
