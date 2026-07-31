///
/// MIDI output via FluidSynth
///

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <fluidsynth.h>

#include "midi.h"
#include "midi_backend.h"

#include "sys/path.h"

static fluid_settings_t *FsSettings = nullptr;
static fluid_synth_t *FsSynth = nullptr;
static fluid_audio_driver_t *FsAudioDriver = nullptr;
static int FsFontId = -1;

static std::vector<std::string> FsFontPaths;
static std::vector<MID_BACKEND_DEVICE_SOURCE> FsFontSources;
static size_t FsFontIndex = 0;

static constexpr int SAMPLE_RATE = 44100;

static std::string_view Basename(std::string_view path) {
  const auto sep = path.find_last_of("/\\");
  return (sep == std::string_view::npos) ? path : path.substr(sep + 1);
}

// File extensions that FluidSynth can load natively.
static constexpr std::string_view FontExts[] = {".sf2", ".sf3", ".dls"};

// Collects font files from [dir] into [paths] + [sources] with the given
// [source] tag. Skips duplicates.
static void ScanDir(const std::string &dir, std::vector<std::string> &paths,
                    std::vector<MID_BACKEND_DEVICE_SOURCE> &sources,
                    MID_BACKEND_DEVICE_SOURCE source) {
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
        sources.push_back(source);
        break;
      }
    }
  }
}

static void ScanSoundFonts(std::string_view data_path) {
  FsFontPaths.clear();
  FsFontSources.clear();

  // 1. System-level paths (higher priority — scanned first so they appear
  //    earlier in the device list).
#ifdef WIN32
  ScanDir("C:/Windows/system32/drivers", FsFontPaths, FsFontSources,
          MID_BACKEND_DEVICE_SOURCE::SYSTEM); // gm.dls
#else
  // Common Linux SoundFont locations
  ScanDir("/usr/share/sounds/sf2", FsFontPaths, FsFontSources,
          MID_BACKEND_DEVICE_SOURCE::SYSTEM);
  ScanDir("/usr/share/soundfonts", FsFontPaths, FsFontSources,
          MID_BACKEND_DEVICE_SOURCE::SYSTEM);
#endif
  // Environment variable override (e.g. DEFAULT_SOUNDFONT=/path/to/font.sf2)
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
  if (const char *env = std::getenv("DEFAULT_SOUNDFONT")) {
    FsFontPaths.push_back(env);
    FsFontSources.push_back(MID_BACKEND_DEVICE_SOURCE::ENV);
  }
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic pop
#endif

  // 2. Local project soundfonts/ directory.
  const std::string sf_dir =
      std::string{data_path.data(), data_path.size()} + "soundfonts";
  ScanDir(sf_dir, FsFontPaths, FsFontSources, MID_BACKEND_DEVICE_SOURCE::LOCAL);

  // Deduplicate and sort. Must keep paths and sources in sync.
  {
    struct Indexed {
      const std::string *path;
      MID_BACKEND_DEVICE_SOURCE source;
      size_t original_index;
    };
    std::vector<Indexed> indexed;
    indexed.reserve(FsFontPaths.size());
    for (size_t i = 0; i < FsFontPaths.size(); i++) {
      indexed.push_back({&FsFontPaths[i], FsFontSources[i], i});
    }
    std::ranges::sort(indexed, [](const Indexed &a, const Indexed &b) {
      return *a.path < *b.path;
    });
    const auto [first, last] =
        std::ranges::unique(indexed, std::ranges::equal_to{},
                            [](const Indexed &x) { return *x.path; });
    indexed.erase(first, last);

    std::vector<std::string> new_paths;
    std::vector<MID_BACKEND_DEVICE_SOURCE> new_sources;
    new_paths.reserve(indexed.size());
    new_sources.reserve(indexed.size());
    for (const auto &entry : indexed) {
      new_paths.push_back(std::move(const_cast<std::string &>(*entry.path)));
      new_sources.push_back(entry.source);
    }
    FsFontPaths = std::move(new_paths);
    FsFontSources = std::move(new_sources);
  }
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
  FsSettings = new_fluid_settings();
  if (!FsSettings) {
    return false;
  }
  fluid_settings_setnum(FsSettings, "synth.sample-rate", SAMPLE_RATE);
  fluid_settings_setint(FsSettings, "synth.audio-channels", 2);
  fluid_settings_setnum(FsSettings, "synth.gain", 1.0);

  FsSynth = new_fluid_synth(FsSettings);
  if (!FsSynth) {
    delete_fluid_settings(FsSettings);
    FsSettings = nullptr;
    return false;
  }

  // Try default driver first, then platform-specific fallbacks.
  FsAudioDriver = new_fluid_audio_driver(FsSettings, FsSynth);
#if !defined(WIN32)
  if (!FsAudioDriver) {
    // On Linux, the default ALSA driver may fail on PipeWire-only systems.
    fluid_settings_setstr(FsSettings, "audio.driver", "pulseaudio");
    FsAudioDriver = new_fluid_audio_driver(FsSettings, FsSynth);
  }
#endif
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
  // Config save is handled by the caller via MidBackend_CurrentSoundFont().
}

bool MidBackend_Init(std::string_view preferred_soundfont) {
  if (FsSynth) {
    return true;
  }

  ScanSoundFonts(PathForData());
  if (FsFontPaths.empty()) {
    return false;
  }

  FsFontIndex = FindSoundFont(preferred_soundfont);
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

std::optional<std::string_view> MidBackend_CurrentSoundFont() {
  if (!FsSynth || FsFontIndex >= FsFontPaths.size()) {
    return std::nullopt;
  }
  static thread_local std::string cached;
  cached = Basename(FsFontPaths[FsFontIndex]);
  return cached;
}

std::optional<std::string_view> MidBackend_DeviceName(void) {
  if (!FsSynth || FsFontIndex >= FsFontPaths.size()) {
    return std::nullopt;
  }
  const auto name = Basename(FsFontPaths[FsFontIndex]);
  static thread_local std::string cached;
  cached = name;
  return cached;
}

size_t MidBackend_DeviceCount(void) { return FsFontPaths.size(); }

std::optional<std::string_view> MidBackend_DeviceNameAt(size_t index) {
  if (index >= FsFontPaths.size()) {
    return std::nullopt;
  }
  const auto name = Basename(FsFontPaths[index]);
  static thread_local std::string cached;
  cached = name;
  return cached;
}

std::optional<MID_BACKEND_DEVICE_SOURCE> MidBackend_DeviceSource(size_t index) {
  if (index >= FsFontSources.size()) {
    return std::nullopt;
  }
  return FsFontSources[index];
}

bool MidBackend_DeviceSelect(size_t index) {
  if (index >= FsFontPaths.size()) {
    return false;
  }
  if (index == FsFontIndex) {
    return true;
  }

  const auto old_index = FsFontIndex;
  FsFontIndex = index;

  // Tear down audio and unload current font.
  if (FsAudioDriver) {
    delete_fluid_audio_driver(FsAudioDriver);
    FsAudioDriver = nullptr;
  }
  fluid_synth_sfunload(FsSynth, FsFontId, 1);
  FsFontId = -1;

  FsFontId = fluid_synth_sfload(FsSynth, FsFontPaths[FsFontIndex].c_str(), 1);
  if (FsFontId == FLUID_FAILED) {
    FsFontIndex = old_index;
    FsFontId = fluid_synth_sfload(FsSynth, FsFontPaths[FsFontIndex].c_str(), 1);
  }

  FsAudioDriver = new_fluid_audio_driver(FsSettings, FsSynth);
  if (!FsAudioDriver) {
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

static constexpr auto TIMER_INTERVAL = std::chrono::milliseconds(10);
static std::jthread FsTimer;

void MidBackend_StartTimer(void) {
  if (FsTimer.joinable() && FsTimer.get_stop_token().stop_requested()) {
    FsTimer.join();
  }

  if (!FsTimer.joinable()) {
    FsTimer = std::jthread([](std::stop_token stop) {
      auto next_tick = std::chrono::steady_clock::now();
      while (!stop.stop_requested()) {
        next_tick += TIMER_INTERVAL;
        std::this_thread::sleep_until(next_tick);
        if (!stop.stop_requested()) {
          Mid_Proc(std::chrono::duration_cast<MID_REALTIME>(TIMER_INTERVAL));
        }
      }
    });
  }
}

void MidBackend_StopTimer(void) {
  if (FsTimer.joinable()) {
    FsTimer.request_stop();
    if (FsTimer.get_id() != std::this_thread::get_id()) {
      FsTimer.join();
    }
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
