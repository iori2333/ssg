///
/// Format-independent background music interface
///

#include <format>

#include <SDL3/SDL_filesystem.h>

#include "audio/bgm.h"
#include "audio/bgm_track.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "audio/snd.h"
#include "audio/snd_backend.h"
#include "audio/volume.h"
#include "sys/file.h"
#include "sys/path.h"

// Direct calls into the game-specific loader, avoiding unnecessary function
// pointer indirection.
bool LoadMusic(unsigned int no);
bool LoadMusicByIndex(int index);
std::string_view MusicTitle(unsigned int index);

using namespace std::chrono_literals;

static constexpr std::string_view BGM_ROOT = "bgm/";

// State
// -----

uint8_t BGM_Tempo_Num = BGM_TEMPO_DENOM;

static bool Enabled = false;
static bool Playing = false;
static bool LoadedOriginalMIDI = false;
static bool GainApply = true;
static unsigned int LoadedNum = 0; // 0 = nothing
static std::chrono::milliseconds MIDITableUpdatePrev;

static std::optional<bool> PacksAvailable = std::nullopt;
static std::string PackPath;
static std::shared_ptr<BGM::TRACK> Waveform; // nullptr = playing MIDI
// -----

// External dependencies
// ---------------------

const uint8_t &Mid_TempoNum = BGM_Tempo_Num;
const uint8_t &Mid_TempoDenom = BGM_TEMPO_DENOM;

const uint8_t &Snd_BGMTempoNum = BGM_Tempo_Num;
const uint8_t &Snd_BGMTempoDenom = BGM_TEMPO_DENOM;
// ---------------------

bool BGM_Init(void) {
  BGM_SetTempo(0);
  Enabled = (MidBackend_Init() | Snd_BGMInit());
  return Enabled;
}

void BGM_Cleanup(void) {
  BGM_Stop();
  MidBackend_Cleanup();
  Snd_BGMCleanup();
  Enabled = false;
}

bool BGM_Enabled(void) { return Enabled; }

bool BGM_LoadedOriginalMIDI(void) { return LoadedOriginalMIDI; }

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
  return Mid_PlayTime.realtime;
}

std::string_view BGM_Title(void) {
  if (Waveform && !Waveform->metadata.title.empty()) {
    return Waveform->metadata.title;
  }
  if ((LoadedNum > 0)) {
    return MusicTitle(LoadedNum - 1);
  }
  return {};
}

bool BGM_ChangeMIDIDevice(int8_t direction) {
  // Stop according to each function's requirements
  Mid_Stop();

  const auto ret = MidBackend_DeviceChange(direction);
  if (ret && Playing && !Waveform) {
    Mid_Play();
  }
  return ret;
}

static bool BGM_Load(unsigned int id) {
  if (!PackPath.empty()) {
    LoadedOriginalMIDI = false;
    const auto prefix_len = PackPath.size();
    PackPath += std::format("{:02}", (id + 1));

    Waveform = BGM::TrackOpen(PackPath);
    if (Waveform && SndBackend_BGMLoad(Waveform)) {
      LoadedOriginalMIDI = LoadMusicByIndex(static_cast<int>(id));
      PackPath.resize(prefix_len);
      return true;
    }

    PackPath.resize(prefix_len);
  }
  LoadedOriginalMIDI = LoadMusic(id);
  return LoadedOriginalMIDI;
}

bool BGM_Switch(unsigned int id) {
  if (!Enabled) {
    return false;
  }
  BGM_Stop();
  Waveform = nullptr;
  const auto ret = BGM_Load(id);
  if (ret) {
    LoadedNum = (id + 1);
    BGM_SetTempo(BGM_GetTempo());
    BGM_Play();
  }
  return ret;
}

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
  // This is called when the window loses focus. Maybe the user will add a
  // BGM pack before coming back?
  PacksAvailable = std::nullopt;

  // Waveform tracks are automatically paused as part of the Snd subsystem
  // once the game window loses focus. We might need independent pausing in
  // the future?
  if (!Waveform) {
    Mid_Pause();
  }
}

void BGM_Resume(void) {
  // Same as for pausing; /s/paus/resum/g, /s/loses/regains/
  if (!Waveform) {
    Mid_Resume();
  }
}

void BGM_UpdateMIDITables(void) {
  if (Waveform && Playing) {
    // This is the only per-frame update function in the current
    // architecture that can actually stop playback at the end of a fade.
    // Coincidentally, the MIDI tables are the only place where this
    // matters, and true stopping could be observed.
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
  const auto volume_start = (volume_cur - 1);

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
  SndBackend_BGMUpdateTempo();
}

static bool BGM_PackIterator(std::invocable<std::string_view> auto callback) {
  return SDL_EnumerateDirectory(
      BGM_ROOT.data(),
      [](void *cb, const char *bgm_root, const char *basename) {
        auto fn = std::format("{}{}", bgm_root, basename);
        if (!PathIsDirectory(fn.c_str())) {
          return SDL_ENUM_CONTINUE;
        }
        return (std::bit_cast<decltype(callback) *>(cb))
            ->operator()(std::string_view{basename});
      },
      &callback);
}

bool BGM_PacksAvailable(bool invalidate_cache) {
  if (PacksAvailable.has_value() && !invalidate_cache) {
    return PacksAvailable.value();
  }
  PacksAvailable =
      BGM_PackIterator([](std::string_view) { return SDL_ENUM_SUCCESS; });
  return PacksAvailable.value();
}

size_t BGM_PackCount(void) {
  size_t ret = 0;
  BGM_PackIterator([&](std::string_view) {
    ret++;
    return SDL_ENUM_CONTINUE;
  });
  return ret;
}

void BGM_PackForeach(std::function<void(std::string_view pack)> func) {
  BGM_PackIterator([&](std::string_view pack) {
    func(pack);
    return SDL_ENUM_CONTINUE;
  });
}

bool BGM_PackSet(std::string_view pack) {
  std::string_view cur = PackPath;
  if (!pack.empty()) {
    const auto path_data = PathForData();
    const auto root_len = (path_data.size() + BGM_ROOT.size());
    if ((cur.size() > root_len) &&
        (cur.substr(root_len, pack.size()) == pack) &&
        (cur[root_len + pack.size()] == '/') // !!!
    ) {
      return true;
    }
    PackPath = std::format("{}{}{}/", path_data, BGM_ROOT, pack);

    // Check if this path exists
    if (!PathIsDirectory(PackPath.c_str())) {
      PackPath.clear();
      return false;
    }
  } else {
    if (cur.empty()) {
      return true;
    }
    PackPath.clear();
  }

  if ((LoadedNum != 0) && Playing) {
    BGM_Switch(LoadedNum - 1);
  }
  return true;
}
