///
/// MIDI output via FluidSynth
///

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL_timer.h>

#include <fluidsynth.h>

#include "midi.h"
#include "midi_backend.h"

#include "core/config.h"
#include "sys/path.h"

static fluid_settings_t *FsSettings = nullptr;
static fluid_synth_t *FsSynth = nullptr;
static fluid_audio_driver_t *FsAudioDriver = nullptr;
static int FsFontId = -1;

static std::vector<std::string> FsFontPaths;
static size_t FsFontIndex = 0;

static constexpr int SAMPLE_RATE = 44100;

static std::string_view Basename(std::string_view path) {
  const auto sep = path.find_last_of("/\\");
  return (sep == std::string_view::npos) ? path : path.substr(sep + 1);
}

// File extensions that FluidSynth can load natively.
static constexpr std::string_view FontExts[] = {".sf2", ".sf3", ".dls"};

// Collects font files from [dir] into [paths], skipping duplicates.
static void ScanDir(const std::string &dir,
                    std::vector<std::string> &paths) {
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) {
    return;
  }
  for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto ext = entry.path().extension().string();
    for (const auto &valid : FontExts) {
      if (ext == valid) {
        paths.push_back(entry.path().string());
        break;
      }
    }
  }
}

static void ScanSoundFonts(std::string_view data_path) {
  FsFontPaths.clear();

  // 1. System-level paths (higher priority — scanned first so they appear
  //    earlier in the device list).
#ifdef WIN32
  ScanDir("C:/Windows/system32/drivers", FsFontPaths); // gm.dls
#else
  // Common Linux SoundFont locations
  ScanDir("/usr/share/sounds/sf2", FsFontPaths);
  ScanDir("/usr/share/soundfonts", FsFontPaths);
#endif
  // Environment variable override (e.g. DEFAULT_SOUNDFONT=/path/to/ font.sf2)
  if (const char *env = SDL_getenv("DEFAULT_SOUNDFONT")) {
    FsFontPaths.push_back(env);
  }

  // 2. Local project soundfonts/ directory.
  const std::string sf_dir =
      std::string{data_path.data(), data_path.size()} + "soundfonts";
  ScanDir(sf_dir, FsFontPaths);

  // Deduplicate and sort.
  std::ranges::sort(FsFontPaths);
  const auto [first, last] = std::ranges::unique(FsFontPaths);
  FsFontPaths.erase(first, last);
}

static size_t FindSoundFont(std::string_view name) {
  if (name.empty()) {
    return SIZE_MAX;
  }
  for (size_t i = 0; i < FsFontPaths.size(); i++) {
    if (Basename(FsFontPaths[i]) == name) {
      return i;
    }
  }
  return SIZE_MAX;
}

static bool FsInitAudio(void) {
  // Settings must be configured before creating the synth (FluidSynth 2.1.0+).
  FsSettings = new_fluid_settings();
  if (!FsSettings) {
    return false;
  }
  fluid_settings_setnum(FsSettings, "synth.sample-rate",
                        static_cast<double>(SAMPLE_RATE));
  fluid_settings_setint(FsSettings, "synth.audio-channels", 2);

  FsSynth = new_fluid_synth(FsSettings);
  if (!FsSynth) {
    delete_fluid_settings(FsSettings);
    FsSettings = nullptr;
    return false;
  }

  FsAudioDriver = new_fluid_audio_driver(FsSettings, FsSynth);
  if (!FsAudioDriver) {
    delete_fluid_synth(FsSynth);
    FsSynth = nullptr;
    delete_fluid_settings(FsSettings);
    FsSettings = nullptr;
    return false;
  }

  return true;
}

static void FsCleanupAudio(void) {
  if (FsAudioDriver) {
    delete_fluid_audio_driver(FsAudioDriver);
    FsAudioDriver = nullptr;
  }
  if (FsSynth) {
    if (FsFontId >= 0) {
      fluid_synth_sfunload(FsSynth, FsFontId, 1);
      FsFontId = -1;
    }
    delete_fluid_synth(FsSynth);
    FsSynth = nullptr;
  }
  if (FsSettings) {
    delete_fluid_settings(FsSettings);
    FsSettings = nullptr;
  }
}

static void FsSaveCurrent(void) {
  if (FsFontIndex < FsFontPaths.size()) {
    ConfigDat.soundfont = Basename(FsFontPaths[FsFontIndex]);
  }
}

bool MidBackend_Init(void) {
  if (FsSynth) {
    return true;
  }

  ScanSoundFonts(PathForData());
  if (FsFontPaths.empty()) {
    return false;
  }

  FsFontIndex = FindSoundFont(ConfigDat.soundfont);
  if (FsFontIndex == SIZE_MAX) {
    FsFontIndex = 0;
    FsSaveCurrent();
  }

  if (!FsInitAudio()) {
    return false;
  }

  FsFontId = fluid_synth_sfload(FsSynth, FsFontPaths[FsFontIndex].c_str(), 1);
  if (FsFontId == FLUID_FAILED) {
    FsCleanupAudio();
    return false;
  }

  return true;
}

void MidBackend_Cleanup(void) { FsCleanupAudio(); }

std::optional<std::string_view> MidBackend_DeviceName(void) {
  if (!FsSynth || FsFontIndex >= FsFontPaths.size()) {
    return std::nullopt;
  }
  const auto name = Basename(FsFontPaths[FsFontIndex]);
  static thread_local std::string cached;
  cached = name;
  return cached;
}

bool MidBackend_DeviceChange(int8_t direction) {
  if (FsFontPaths.size() <= 1) {
    return true; // No other SoundFont to switch to, but don't stop playback
  }

  const auto old_index = FsFontIndex;
  if (direction > 0) {
    FsFontIndex = (FsFontIndex + 1) % FsFontPaths.size();
  } else {
    FsFontIndex = (FsFontIndex + FsFontPaths.size() - 1) % FsFontPaths.size();
  }

  // Tear down audio and unload current font.
  if (FsAudioDriver) {
    delete_fluid_audio_driver(FsAudioDriver);
    FsAudioDriver = nullptr;
  }
  fluid_synth_sfunload(FsSynth, FsFontId, 1);
  FsFontId = -1;

  // Load new SoundFont.
  FsFontId = fluid_synth_sfload(FsSynth, FsFontPaths[FsFontIndex].c_str(), 1);
  if (FsFontId == FLUID_FAILED) {
    // Roll back to the old font.
    FsFontIndex = old_index;
    FsFontId = fluid_synth_sfload(FsSynth, FsFontPaths[FsFontIndex].c_str(), 1);
  }

  // Restart audio.
  FsAudioDriver = new_fluid_audio_driver(FsSettings, FsSynth);
  if (!FsAudioDriver) {
    // If audio restart fails, tear down everything and roll back to the old
    // font.
    FsCleanupAudio();
    FsFontIndex = old_index;
    if (FsInitAudio()) {
      FsFontId =
          fluid_synth_sfload(FsSynth, FsFontPaths[FsFontIndex].c_str(), 1);
      FsAudioDriver = new_fluid_audio_driver(FsSettings, FsSynth);
    }
    return false;
  }

  FsSaveCurrent();
  return true;
}

static SDL_TimerID FsTimer = 0;

static constexpr auto TIMER_INTERVAL = std::chrono::milliseconds(10);

extern "C" uint32_t TimerCallback(void *, SDL_TimerID, uint32_t interval) {
  Mid_Proc(std::chrono::duration_cast<MID_REALTIME>(
      std::chrono::milliseconds{interval}));
  return static_cast<uint32_t>(TIMER_INTERVAL.count());
}

void MidBackend_StartTimer(void) {
  if (!FsTimer) {
    FsTimer = SDL_AddTimer(TIMER_INTERVAL.count(), TimerCallback, nullptr);
  }
}

void MidBackend_StopTimer(void) {
  if (FsTimer) {
    SDL_RemoveTimer(FsTimer);
    FsTimer = 0;
  }
}

void MidBackend_Out(uint8_t status, uint8_t a, uint8_t b) {
  if (!FsSynth) {
    return;
  }

  const int ch = (status & 0x0F);

  switch (status & 0xF0) {
  case 0x80: // Note Off
    fluid_synth_noteoff(FsSynth, ch, a);
    break;

  case 0x90: // Note On
    if (b == 0) {
      fluid_synth_noteoff(FsSynth, ch, a);
    } else {
      fluid_synth_noteon(FsSynth, ch, a, b);
    }
    break;

  case 0xA0: // Polyphonic Aftertouch
    fluid_synth_key_pressure(FsSynth, ch, a, b);
    break;

  case 0xB0: // Control Change
    fluid_synth_cc(FsSynth, ch, a, b);
    break;

  case 0xC0: // Program Change
    fluid_synth_program_change(FsSynth, ch, a);
    break;

  case 0xD0: // Channel Aftertouch
    fluid_synth_channel_pressure(FsSynth, ch, a);
    break;

  case 0xE0: { // Pitch Bend
    const int val = (a | (b << 7));
    fluid_synth_pitch_bend(FsSynth, ch, val);
    break;
  }
  }
}

void MidBackend_Out(std::span<uint8_t> event) {
  if (event.size() < 1) {
    return;
  }

  // Route SysEx messages to fluid_synth_sysex().
  if (event[0] == 0xF0) {
    if (!FsSynth) {
      return;
    }
    // The data is everything after the leading 0xF0 byte.
    const char *data = reinterpret_cast<const char *>(event.data() + 1);
    const int len = static_cast<int>(event.size() - 1);
    fluid_synth_sysex(FsSynth, data, len, nullptr, nullptr, nullptr, 0);
    return;
  }

  uint8_t a = (event.size() >= 2) ? event[1] : 0;
  uint8_t b = (event.size() >= 3) ? event[2] : 0;
  MidBackend_Out(event[0], a, b);
}

void MidBackend_Panic(void) {
  if (!FsSynth) {
    return;
  }
  // -1 means "all channels"
  fluid_synth_all_sounds_off(FsSynth, -1);
}
