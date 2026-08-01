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

#include "sys/log.h"
#include "sys/path.h"

namespace {

struct FluidSynthState {
  fluid_settings_t *settings = nullptr;
  fluid_synth_t *synth = nullptr;
  fluid_audio_driver_t *audio_driver = nullptr;
  int font_id = -1;
  std::vector<std::string> font_paths;
  std::vector<MidiDeviceSource> font_sources;
  size_t font_index = 0;
  std::jthread timer;
};

FluidSynthState &State() {
  static FluidSynthState state;
  return state;
}

constexpr int kSampleRate = 44100;
constexpr auto kTimerInterval = std::chrono::milliseconds(10);

} // namespace

static std::string_view Basename(std::string_view path) {
  const auto sep = path.find_last_of("/\\");
  return (sep == std::string_view::npos) ? path : path.substr(sep + 1);
}

// File extensions that FluidSynth can load natively.
static constexpr std::string_view kFontExtensions[] = {".sf2", ".sf3", ".dls"};

// Collects font files from [dir] into [paths] + [sources] with the given
// [source] tag. Skips duplicates.
static void ScanDir(const std::string &dir, std::vector<std::string> &paths,
                    std::vector<MidiDeviceSource> &sources,
                    MidiDeviceSource source) {
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) {
    return;
  }
  for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto ext = entry.path().extension().string();
    for (const auto &valid : kFontExtensions) {
      if (ext == valid) {
        paths.push_back(entry.path().string());
        sources.push_back(source);
        break;
      }
    }
  }
}

static void ScanSoundFonts(std::string_view data_path) {
  State().font_paths.clear();
  State().font_sources.clear();

  // 1. System-level paths (higher priority — scanned first so they appear
  //    earlier in the device list).
#ifdef WIN32
  ScanDir("C:/Windows/system32/drivers", State().font_paths,
          State().font_sources,
          MidiDeviceSource::System); // gm.dls
#else
  // Common Linux SoundFont locations
  ScanDir("/usr/share/sounds/sf2", State().font_paths, State().font_sources,
          MidiDeviceSource::System);
  ScanDir("/usr/share/soundfonts", State().font_paths, State().font_sources,
          MidiDeviceSource::System);
#endif
  // Environment variable override (e.g. DEFAULT_SOUNDFONT=/path/to/font.sf2)
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
  if (const char *env = std::getenv("DEFAULT_SOUNDFONT")) {
    State().font_paths.push_back(env);
    State().font_sources.push_back(MidiDeviceSource::Environment);
  }
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic pop
#endif

  // 2. Local project soundfonts/ directory.
  const std::string sf_dir =
      std::string{data_path.data(), data_path.size()} + "soundfonts";
  ScanDir(sf_dir, State().font_paths, State().font_sources,
          MidiDeviceSource::Local);

  // Deduplicate and sort. Must keep paths and sources in sync.
  {
    struct Indexed {
      const std::string *path;
      MidiDeviceSource source;
      size_t original_index;
    };
    std::vector<Indexed> indexed;
    indexed.reserve(State().font_paths.size());
    for (size_t i = 0; i < State().font_paths.size(); i++) {
      indexed.push_back({&State().font_paths[i], State().font_sources[i], i});
    }
    std::ranges::sort(indexed, [](const Indexed &a, const Indexed &b) {
      return *a.path < *b.path;
    });
    const auto [first, last] =
        std::ranges::unique(indexed, std::ranges::equal_to{},
                            [](const Indexed &x) { return *x.path; });
    indexed.erase(first, last);

    std::vector<std::string> new_paths;
    std::vector<MidiDeviceSource> new_sources;
    new_paths.reserve(indexed.size());
    new_sources.reserve(indexed.size());
    for (const auto &entry : indexed) {
      new_paths.push_back(std::move(const_cast<std::string &>(*entry.path)));
      new_sources.push_back(entry.source);
    }
    State().font_paths = std::move(new_paths);
    State().font_sources = std::move(new_sources);
  }
}

static size_t FindSoundFont(std::string_view name) {
  if (name.empty()) {
    return SIZE_MAX;
  }
  for (size_t i = 0; i < State().font_paths.size(); i++) {
    if (Basename(State().font_paths[i]) == name) {
      return i;
    }
  }
  return SIZE_MAX;
}

static bool FsInitAudio() {
  State().settings = new_fluid_settings();
  if (!State().settings) {
    return false;
  }
  fluid_settings_setnum(State().settings, "synth.sample-rate", kSampleRate);
  fluid_settings_setint(State().settings, "synth.audio-channels", 2);
  fluid_settings_setnum(State().settings, "synth.gain", 1.0);

  State().synth = new_fluid_synth(State().settings);
  if (!State().synth) {
    delete_fluid_settings(State().settings);
    State().settings = nullptr;
    return false;
  }

  // Try default driver first, then platform-specific fallbacks.
  State().audio_driver =
      new_fluid_audio_driver(State().settings, State().synth);
#if !defined(WIN32)
  if (!State().audio_driver) {
    // On Linux, the default ALSA driver may fail on PipeWire-only systems.
    fluid_settings_setstr(State().settings, "audio.driver", "pulseaudio");
    State().audio_driver =
        new_fluid_audio_driver(State().settings, State().synth);
  }
#endif
  if (!State().audio_driver) {
    delete_fluid_synth(State().synth);
    State().synth = nullptr;
    delete_fluid_settings(State().settings);
    State().settings = nullptr;
    return false;
  }

  return true;
}

static void FsCleanupAudio() {
  if (State().audio_driver) {
    delete_fluid_audio_driver(State().audio_driver);
    State().audio_driver = nullptr;
  }
  if (State().synth) {
    if (State().font_id >= 0) {
      fluid_synth_sfunload(State().synth, State().font_id, 1);
      State().font_id = -1;
    }
    delete_fluid_synth(State().synth);
    State().synth = nullptr;
  }
  if (State().settings) {
    delete_fluid_settings(State().settings);
    State().settings = nullptr;
  }
}

bool MidiBackendInitialize(std::string_view preferred_soundfont) {
  if (State().synth) {
    return true;
  }

  ScanSoundFonts(PathForData());
  if (State().font_paths.empty()) {
    logging::Error(logging::Channel::Audio, "No SoundFont files were found");
    return false;
  }

  State().font_index = FindSoundFont(preferred_soundfont);
  if (State().font_index == SIZE_MAX) {
    State().font_index = 0;
  }

  if (!FsInitAudio()) {
    logging::Error(logging::Channel::Audio,
                   "Failed to initialize FluidSynth audio");
    return false;
  }

  State().font_id = fluid_synth_sfload(
      State().synth, State().font_paths[State().font_index].c_str(), 1);
  if (State().font_id == FLUID_FAILED) {
    logging::Error(logging::Channel::Audio, "Failed to load SoundFont: {}",
                   State().font_paths[State().font_index]);
    FsCleanupAudio();
    return false;
  }

  logging::Info(logging::Channel::Audio, "Using SoundFont: {}",
                Basename(State().font_paths[State().font_index]));
  return true;
}

void MidiBackendCleanup() { FsCleanupAudio(); }

std::optional<std::string_view> MidiBackendCurrentSoundFont() {
  if (!State().synth || State().font_index >= State().font_paths.size()) {
    return std::nullopt;
  }
  static thread_local std::string cached;
  cached = Basename(State().font_paths[State().font_index]);
  return cached;
}

std::optional<std::string_view> MidiBackendDeviceName() {
  if (!State().synth || State().font_index >= State().font_paths.size()) {
    return std::nullopt;
  }
  const auto name = Basename(State().font_paths[State().font_index]);
  static thread_local std::string cached;
  cached = name;
  return cached;
}

size_t MidiBackendDeviceCount() { return State().font_paths.size(); }

std::optional<std::string_view> MidiBackendDeviceNameAt(size_t index) {
  if (index >= State().font_paths.size()) {
    return std::nullopt;
  }
  const auto name = Basename(State().font_paths[index]);
  static thread_local std::string cached;
  cached = name;
  return cached;
}

std::optional<MidiDeviceSource> MidiBackendDeviceSource(size_t index) {
  if (index >= State().font_sources.size()) {
    return std::nullopt;
  }
  return State().font_sources[index];
}

bool MidiBackendSelectDevice(size_t index) {
  if (index >= State().font_paths.size()) {
    return false;
  }
  if (index == State().font_index) {
    return true;
  }

  const auto old_index = State().font_index;
  State().font_index = index;

  // Tear down audio and unload current font.
  if (State().audio_driver) {
    delete_fluid_audio_driver(State().audio_driver);
    State().audio_driver = nullptr;
  }
  fluid_synth_sfunload(State().synth, State().font_id, 1);
  State().font_id = -1;

  State().font_id = fluid_synth_sfload(
      State().synth, State().font_paths[State().font_index].c_str(), 1);
  if (State().font_id == FLUID_FAILED) {
    State().font_index = old_index;
    State().font_id = fluid_synth_sfload(
        State().synth, State().font_paths[State().font_index].c_str(), 1);
  }

  State().audio_driver =
      new_fluid_audio_driver(State().settings, State().synth);
  if (!State().audio_driver) {
    FsCleanupAudio();
    State().font_index = old_index;
    if (FsInitAudio()) {
      State().font_id = fluid_synth_sfload(
          State().synth, State().font_paths[State().font_index].c_str(), 1);
      State().audio_driver =
          new_fluid_audio_driver(State().settings, State().synth);
    }
    return false;
  }

  return true;
}

bool MidiBackendChangeDevice(int8_t direction) {
  if (State().font_paths.size() <= 1) {
    return true; // No other SoundFont to switch to, but don't stop playback
  }

  const auto old_index = State().font_index;
  if (direction > 0) {
    State().font_index = (State().font_index + 1) % State().font_paths.size();
  } else {
    State().font_index = (State().font_index + State().font_paths.size() - 1) %
                         State().font_paths.size();
  }

  // Tear down audio and unload current font.
  if (State().audio_driver) {
    delete_fluid_audio_driver(State().audio_driver);
    State().audio_driver = nullptr;
  }
  fluid_synth_sfunload(State().synth, State().font_id, 1);
  State().font_id = -1;

  // Load new SoundFont.
  State().font_id = fluid_synth_sfload(
      State().synth, State().font_paths[State().font_index].c_str(), 1);
  if (State().font_id == FLUID_FAILED) {
    // Roll back to the old font.
    State().font_index = old_index;
    State().font_id = fluid_synth_sfload(
        State().synth, State().font_paths[State().font_index].c_str(), 1);
  }

  // Restart audio.
  State().audio_driver =
      new_fluid_audio_driver(State().settings, State().synth);
  if (!State().audio_driver) {
    // If audio restart fails, tear down everything and roll back to the old
    // font.
    FsCleanupAudio();
    State().font_index = old_index;
    if (FsInitAudio()) {
      State().font_id = fluid_synth_sfload(
          State().synth, State().font_paths[State().font_index].c_str(), 1);
      State().audio_driver =
          new_fluid_audio_driver(State().settings, State().synth);
    }
    return false;
  }

  return true;
}

void MidiBackendStartTimer() {
  if (State().timer.joinable() &&
      State().timer.get_stop_token().stop_requested()) {
    State().timer.join();
  }

  if (!State().timer.joinable()) {
    State().timer = std::jthread([](std::stop_token stop) {
      auto next_tick = std::chrono::steady_clock::now();
      while (!stop.stop_requested()) {
        next_tick += kTimerInterval;
        std::this_thread::sleep_until(next_tick);
        if (!stop.stop_requested()) {
          MidiProcess(std::chrono::duration_cast<MidiRealtime>(kTimerInterval));
        }
      }
    });
  }
}

void MidiBackendStopTimer() {
  if (State().timer.joinable()) {
    State().timer.request_stop();
    if (State().timer.get_id() != std::this_thread::get_id()) {
      State().timer.join();
    }
  }
}

void MidiBackendOutput(uint8_t status, uint8_t a, uint8_t b) {
  if (!State().synth) {
    return;
  }

  const int ch = (status & 0x0F);

  switch (status & 0xF0) {
  case 0x80: // Note Off
    fluid_synth_noteoff(State().synth, ch, a);
    break;

  case 0x90: // Note On
    if (b == 0) {
      fluid_synth_noteoff(State().synth, ch, a);
    } else {
      fluid_synth_noteon(State().synth, ch, a, b);
    }
    break;

  case 0xA0: // Polyphonic Aftertouch
    fluid_synth_key_pressure(State().synth, ch, a, b);
    break;

  case 0xB0: // Control Change
    fluid_synth_cc(State().synth, ch, a, b);
    break;

  case 0xC0: // Program Change
    fluid_synth_program_change(State().synth, ch, a);
    break;

  case 0xD0: // Channel Aftertouch
    fluid_synth_channel_pressure(State().synth, ch, a);
    break;

  case 0xE0: { // Pitch Bend
    const int val = (a | (b << 7));
    fluid_synth_pitch_bend(State().synth, ch, val);
    break;
  }
  }
}

void MidiBackendOutput(std::span<uint8_t> event) {
  if (event.size() < 1) {
    return;
  }

  // Route SysEx messages to fluid_synth_sysex().
  if (event[0] == 0xF0) {
    if (!State().synth) {
      return;
    }
    // The data is everything after the leading 0xF0 byte.
    const char *data = reinterpret_cast<const char *>(event.data() + 1);
    const int len = static_cast<int>(event.size() - 1);
    fluid_synth_sysex(State().synth, data, len, nullptr, nullptr, nullptr, 0);
    return;
  }

  uint8_t a = (event.size() >= 2) ? event[1] : 0;
  uint8_t b = (event.size() >= 3) ? event[2] : 0;
  MidiBackendOutput(event[0], a, b);
}

void MidiBackendPanic() {
  if (!State().synth) {
    return;
  }
  // -1 means "all channels"
  fluid_synth_all_sounds_off(State().synth, -1);
}
