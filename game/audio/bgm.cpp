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

uint8_t BGM_Tempo_Num = BGM_TEMPO_DENOM;

static bool Enabled = false;
static bool Playing = false;
static bool GainApply = true;
static unsigned int LoadedNum = 0;
static std::chrono::milliseconds MIDITableUpdatePrev;

static std::shared_ptr<BGM::TRACK> Waveform; // nullptr = playing MIDI
static std::string_view WaveformTitle;       // cached from waveform metadata
// -----

// External dependencies for MIDI tempo — exposed as global references
// ---------------------

const uint8_t &Snd_BGMTempoNum = BGM_Tempo_Num;
const uint8_t &Snd_BGMTempoDenom = BGM_TEMPO_DENOM;
// ---------------------

bool BGM_Init(std::string_view preferred_soundfont) {
  BGM_SetTempo(0);
  Enabled = (MidBackend_Init(preferred_soundfont) | Snd_BGMInit());
  return Enabled;
}

void BGM_Cleanup(void) {
  BGM_Stop();
  MidBackend_Cleanup();
  Snd_BGMCleanup();
  Enabled = false;
}

bool BGM_Enabled(void) { return Enabled; }

bool BGM_HasGainFactor(void) {
  return (Waveform && Waveform->metadata.gain_factor.has_value());
}

bool BGM_GainApply(void) { return GainApply; }

BGM_PLAYING BGM_Playing(void) {
  return ((!Enabled || !Playing) ? BGM_PLAYING::NONE
          : Waveform             ? BGM_PLAYING::WAVEFORM
                                 : BGM_PLAYING::MIDI);
}

std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void) {
  if (Waveform) {
    return SndBackend_BGMPlayTime();
  }
  return Mid_GetPlayTime().realtime;
}

bool BGM_ChangeMIDIDevice(int8_t direction) {
  Mid_Stop();

  const auto ret = MidBackend_DeviceChange(direction);
  if (ret && Playing && !Waveform) {
    Mid_Play();
  }
  return ret;
}

// --- Audio source loading ---

bool BGM_LoadWaveform(std::string_view path) {
  Waveform = BGM::TrackOpen(path);
  if (Waveform && SndBackend_BGMLoad(Waveform)) {
    WaveformTitle = Waveform->metadata.title;
    return true;
  }
  Waveform = nullptr;
  return false;
}

bool BGM_LoadMIDI(std::vector<uint8_t> buf) {
  Waveform = nullptr;
  return Mid_Load(std::move(buf));
}

std::string_view BGM_WaveformTitle() { return WaveformTitle; }

unsigned int BGM_LoadedNum() { return LoadedNum; }

void BGM_SetLoadedNum(unsigned int n) { LoadedNum = n; }

void BGM_ClearWaveform() { Waveform = nullptr; }

// --- Playback ---

void BGM_Play(void) {
  BGM_SetGainApply(GainApply);
  if (Waveform) {
    SndBackend_BGMPlay();
    MIDITableUpdatePrev = decltype(MIDITableUpdatePrev)::zero();
  } else {
    Mid_Play();
  }
  Playing = true;
}

void BGM_Stop(void) {
  if (Waveform) {
    SndBackend_BGMStop();

    // Just in case MIDI is running in update-only mode and the tables are
    // being observed. Would regularly be called by Mid_Stop().
    Mid_TableInit();
  } else {
    Mid_Stop();
  }
  Playing = false;
}

void BGM_Pause(void) {
  if (Waveform) {
    SndBackend_BGMStop();
  } else {
    Mid_Pause();
  }
}

void BGM_Resume(void) {
  if (Waveform) {
    SndBackend_BGMPlay();
  } else {
    Mid_Resume();
  }
}

void BGM_UpdateMIDITables(void) {
  if (Waveform && Playing) {
    if (Waveform->FadeVolumeLinear() <= 0.0f) {
      BGM_Stop();
      return;
    }

    const auto now = SndBackend_BGMPlayTime();
    const auto delta = (now - MIDITableUpdatePrev);
    MIDITableUpdatePrev = now;
    Mid_Proc(delta);
  }
}

void BGM_SetGainApply(bool apply) {
  GainApply = apply;
  if (Waveform) {
    Snd_BGMGainFactor =
        (apply ? Waveform->metadata.gain_factor.value_or(1.0f) : 1.0f);
    SndBackend_BGMUpdateVolume();
  }
}

void BGM_UpdateVolume(void) {
  if (Waveform) {
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
      (Waveform ? VolumeDiscrete(Waveform->FadeVolumeLinear())
                : Mid_GetFadeVolume());
  const auto volume_start = (volume_cur == 0) ? 0 : (volume_cur - 1);

  const auto duration =
      (10ms * VOLUME_MAX * ((((256 - speed) * 4) / (VOLUME_MAX + 1)) + 1));
  if (Waveform) {
    Waveform->FadeOut(VolumeLinear(volume_start), duration);
  } else {
    Mid_FadeOut(volume_start, duration);
  }
}

int8_t BGM_GetTempo(void) { return (BGM_Tempo_Num - BGM_TEMPO_DENOM); }

void BGM_SetTempo(int8_t tempo) {
  tempo = std::clamp(tempo, BGM_TEMPO_MIN, BGM_TEMPO_MAX);
  BGM_Tempo_Num = (BGM_TEMPO_DENOM + tempo);
  Mid_SetTempo(BGM_Tempo_Num, BGM_TEMPO_DENOM);
  SndBackend_BGMUpdateTempo();
}
