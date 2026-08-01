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
  std::shared_ptr<BGM::TRACK> waveform;
  std::string_view waveform_title;
};

BgmState &State() {
  static BgmState state;
  return state;
}

} // namespace

bool BGM_Init(std::string_view preferred_soundfont) {
  BGM_SetTempo(0);
  State().enabled = (MidBackend_Init(preferred_soundfont) | Snd_BGMInit());
  return State().enabled;
}

void BGM_Cleanup(void) {
  BGM_Stop();
  MidBackend_Cleanup();
  Snd_BGMCleanup();
  State().enabled = false;
}

bool BGM_Enabled(void) { return State().enabled; }

bool BGM_HasGainFactor(void) {
  return (State().waveform &&
          State().waveform->metadata.gain_factor.has_value());
}

bool BGM_GainApply(void) { return State().apply_gain; }

BGM_PLAYING BGM_Playing(void) {
  return ((!State().enabled || !State().playing) ? BGM_PLAYING::NONE
          : State().waveform                     ? BGM_PLAYING::WAVEFORM
                                                 : BGM_PLAYING::MIDI);
}

std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void) {
  if (State().waveform) {
    return SndBackend_BGMPlayTime();
  }
  return Mid_GetPlayTime().realtime;
}

bool BGM_ChangeMIDIDevice(int8_t direction) {
  Mid_Stop();

  const auto ret = MidBackend_DeviceChange(direction);
  if (ret && State().playing && !State().waveform) {
    Mid_Play();
  }
  return ret;
}

// --- Audio source loading ---

bool BGM_LoadWaveform(std::string_view path) {
  State().waveform = BGM::TrackOpen(path);
  if (State().waveform && SndBackend_BGMLoad(State().waveform)) {
    State().waveform_title = State().waveform->metadata.title;
    return true;
  }
  State().waveform = nullptr;
  return false;
}

bool BGM_LoadMIDI(std::vector<uint8_t> buf) {
  State().waveform = nullptr;
  return Mid_Load(std::move(buf));
}

std::string_view BGM_WaveformTitle() { return State().waveform_title; }

unsigned int BGM_LoadedNum() { return State().loaded_number; }

void BGM_SetLoadedNum(unsigned int n) { State().loaded_number = n; }

void BGM_ClearWaveform() { State().waveform = nullptr; }

// --- Playback ---

void BGM_Play(void) {
  BGM_SetGainApply(State().apply_gain);
  if (State().waveform) {
    SndBackend_BGMPlay();
    State().midi_table_update_previous =
        decltype(State().midi_table_update_previous)::zero();
  } else {
    Mid_Play();
  }
  State().playing = true;
}

void BGM_Stop(void) {
  if (State().waveform) {
    SndBackend_BGMStop();

    // Just in case MIDI is running in update-only mode and the tables are
    // being observed. Would regularly be called by Mid_Stop().
    Mid_TableInit();
  } else {
    Mid_Stop();
  }
  State().playing = false;
}

void BGM_Pause(void) {
  if (State().waveform) {
    SndBackend_BGMStop();
  } else {
    Mid_Pause();
  }
}

void BGM_Resume(void) {
  if (State().waveform) {
    SndBackend_BGMPlay();
  } else {
    Mid_Resume();
  }
}

void BGM_UpdateMIDITables(void) {
  if (State().waveform && State().playing) {
    if (State().waveform->FadeVolumeLinear() <= 0.0f) {
      BGM_Stop();
      return;
    }

    const auto now = SndBackend_BGMPlayTime();
    const auto delta = (now - State().midi_table_update_previous);
    State().midi_table_update_previous = now;
    Mid_Proc(delta);
  }
}

void BGM_SetGainApply(bool apply) {
  State().apply_gain = apply;
  if (State().waveform) {
    SndBackend_BGMUpdateVolume();
  }
}

float BGM_GainFactor() {
  if (!State().apply_gain || !State().waveform) {
    return 1.0f;
  }
  return State().waveform->metadata.gain_factor.value_or(1.0f);
}

void BGM_UpdateVolume(void) {
  if (State().waveform) {
    SndBackend_BGMUpdateVolume();
  } else {
    Mid_UpdateVolume();
  }
}

void BGM_FadeOut(uint8_t speed) {
  // pbg quirk: The original game always reduced the volume by 1 on the first
  // call to the MIDI FadeIO() method after the start of the fade. This
  // allowed you to hold the fade button in the Music Room for a faster
  // fade-out.
  const VOLUME volume_cur =
      (State().waveform ? VolumeDiscrete(State().waveform->FadeVolumeLinear())
                        : Mid_GetFadeVolume());
  const auto volume_start = (volume_cur == 0) ? 0 : (volume_cur - 1);

  const auto duration =
      (10ms * VOLUME_MAX * ((((256 - speed) * 4) / (VOLUME_MAX + 1)) + 1));
  if (State().waveform) {
    State().waveform->FadeOut(VolumeLinear(volume_start), duration);
  } else {
    Mid_FadeOut(volume_start, duration);
  }
}

int8_t BGM_GetTempo(void) {
  return (State().tempo_numerator - kBgmTempoDenominator);
}

void BGM_SetTempo(int8_t tempo) {
  tempo = std::clamp(tempo, kBgmTempoMin, kBgmTempoMax);
  State().tempo_numerator = (kBgmTempoDenominator + tempo);
  Mid_SetTempo(State().tempo_numerator, kBgmTempoDenominator);
  SndBackend_BGMUpdateTempo();
}

float BGM_TempoFactor() {
  return static_cast<float>(State().tempo_numerator) / kBgmTempoDenominator;
}
