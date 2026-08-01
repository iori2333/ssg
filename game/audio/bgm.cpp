///
/// Format-independent background music interface — pure audio playback
///
/// Track selection, title lookup, and BGM pack management live in
/// GIAN07/music/ - this module only plays what it is given.
///

#include <algorithm>
#include <chrono>
#include <utility>

#include "audio/bgm.h"
#include "audio/bgm_track.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "audio/snd.h"
#include "audio/snd_backend.h"
#include "audio/volume.h"

using namespace std::chrono_literals;

// State
// -----

namespace {

struct BgmState {
  uint8_t tempo_numerator = kBgmTempoDenominator;
  bool enabled = false;
  bool playing = false;
  bool apply_gain = true;
  unsigned int loaded_number = 0;
  std::chrono::milliseconds midi_table_update_previous;
  std::shared_ptr<bgm::Track> waveform;
  std::string_view waveform_title;
};

BgmState &State() {
  static BgmState state;
  return state;
}

} // namespace

bool BgmInitialize(std::string_view preferred_soundfont) {
  BgmSetTempo(0);
  State().enabled =
      (MidiBackendInitialize(preferred_soundfont) | AudioInitializeBgm());
  return State().enabled;
}

void BgmCleanup(void) {
  BgmStop();
  MidiBackendCleanup();
  AudioCleanupBgm();
  State().enabled = false;
}

bool BgmIsEnabled(void) { return State().enabled; }

bool BgmHasGainFactor(void) {
  return (State().waveform &&
          State().waveform->metadata.gain_factor.has_value());
}

bool BgmIsGainApplied(void) { return State().apply_gain; }

BgmPlaybackSource BgmPlayingSource(void) {
  return ((!State().enabled || !State().playing) ? BgmPlaybackSource::None
          : State().waveform                     ? BgmPlaybackSource::Waveform
                                                 : BgmPlaybackSource::Midi);
}

std::chrono::duration<int32_t, std::milli> BgmPlayTime(void) {
  if (State().waveform) {
    return AudioBackendBgmPlayTime();
  }
  return MidiPlayTimePosition().realtime;
}

bool BgmChangeMidiDevice(int8_t direction) {
  MidiStop();

  const auto ret = MidiBackendChangeDevice(direction);
  if (ret && State().playing && !State().waveform) {
    MidiPlay();
  }
  return ret;
}

// --- Audio source loading ---

bool BgmLoadWaveform(std::string_view path) {
  State().waveform = bgm::OpenTrack(path);
  if (State().waveform && AudioBackendLoadBgm(State().waveform)) {
    State().waveform_title = State().waveform->metadata.title;
    return true;
  }
  State().waveform = nullptr;
  return false;
}

bool BgmLoadMidi(std::vector<uint8_t> buf) {
  State().waveform = nullptr;
  return MidiLoad(std::move(buf));
}

std::string_view BgmWaveformTitle() { return State().waveform_title; }

unsigned int BgmLoadedTrackNumber() { return State().loaded_number; }

void BgmSetLoadedTrackNumber(unsigned int n) { State().loaded_number = n; }

void BgmClearWaveform() { State().waveform = nullptr; }

// --- Playback ---

void BgmPlay(void) {
  BgmSetGainApplied(State().apply_gain);
  if (State().waveform) {
    AudioBackendPlayBgm();
    State().midi_table_update_previous =
        decltype(State().midi_table_update_previous)::zero();
  } else {
    MidiPlay();
  }
  State().playing = true;
}

void BgmStop(void) {
  if (State().waveform) {
    AudioBackendStopBgm();

    // Just in case MIDI is running in update-only mode and the tables are
    // being observed. Would regularly be called by MidiStop().
    MidiResetTables();
  } else {
    MidiStop();
  }
  State().playing = false;
}

void BgmPause(void) {
  if (State().waveform) {
    AudioBackendStopBgm();
  } else {
    MidiPause();
  }
}

void BgmResume(void) {
  if (State().waveform) {
    AudioBackendPlayBgm();
  } else {
    MidiResume();
  }
}

void BgmUpdateMidiTables(void) {
  if (State().waveform && State().playing) {
    if (State().waveform->FadeVolumeLinear() <= 0.0f) {
      BgmStop();
      return;
    }

    const auto now = AudioBackendBgmPlayTime();
    const auto delta = (now - State().midi_table_update_previous);
    State().midi_table_update_previous = now;
    MidiProcess(delta);
  }
}

void BgmSetGainApplied(bool apply) {
  State().apply_gain = apply;
  if (State().waveform) {
    AudioBackendUpdateBgmVolume();
  }
}

float BgmGainFactor() {
  if (!State().apply_gain || !State().waveform) {
    return 1.0f;
  }
  return State().waveform->metadata.gain_factor.value_or(1.0f);
}

void BgmUpdateVolume(void) {
  if (State().waveform) {
    AudioBackendUpdateBgmVolume();
  } else {
    MidiUpdateVolume();
  }
}

void BgmFadeOut(uint8_t speed) {
  // pbg quirk: The original game always reduced the volume by 1 on the first
  // call to the MIDI FadeIO() method after the start of the fade. This
  // allowed you to hold the fade button in the Music Room for a faster
  // fade-out.
  const AudioVolume volume_cur =
      (State().waveform
           ? LinearToAudioVolume(State().waveform->FadeVolumeLinear())
           : MidiFadeVolume());
  const auto volume_start = (volume_cur == 0) ? 0 : (volume_cur - 1);

  const auto duration = (10ms * kMaxAudioVolume *
                         ((((256 - speed) * 4) / (kMaxAudioVolume + 1)) + 1));
  if (State().waveform) {
    State().waveform->FadeOut(AudioVolumeToLinear(volume_start), duration);
  } else {
    MidiFadeOut(volume_start, duration);
  }
}

int8_t BgmTempo(void) {
  return (State().tempo_numerator - kBgmTempoDenominator);
}

void BgmSetTempo(int8_t tempo) {
  tempo = std::clamp(tempo, kBgmTempoMin, kBgmTempoMax);
  State().tempo_numerator = (kBgmTempoDenominator + tempo);
  MidiSetTempo(State().tempo_numerator, kBgmTempoDenominator);
  AudioBackendUpdateBgmTempo();
}

float BgmTempoFactor() {
  return static_cast<float>(State().tempo_numerator) / kBgmTempoDenominator;
}
