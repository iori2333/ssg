#include "midi_synth.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fluidsynth.h>

#include "audio/core/audio_types.h"
#include "sys/log.h"

namespace audio::bgm {
namespace {

constexpr std::string_view kFontExtensions[] = {".sf2", ".sf3", ".dls"};
constexpr int kSampleRate = 44100;

std::string Basename(std::string_view path) {
  const auto separator = path.find_last_of("/\\");
  const auto name =
      (separator == std::string_view::npos) ? path : path.substr(separator + 1);
  return std::string{name};
}

void ScanDirectory(const std::string &directory,
                   std::vector<std::string> &paths,
                   std::vector<DeviceSource> &sources, DeviceSource source) {
  std::error_code error;
  if (!std::filesystem::is_directory(directory, error)) {
    return;
  }
  for (const auto &entry :
       std::filesystem::directory_iterator(directory, error)) {
    if (!entry.is_regular_file(error)) {
      continue;
    }
    const auto extension = entry.path().extension().string();
    if (std::ranges::find(kFontExtensions, extension) !=
        std::end(kFontExtensions)) {
      paths.push_back(entry.path().string());
      sources.push_back(source);
    }
  }
}

} // namespace

struct MidiSynth::Impl {
  mutable std::mutex mutex;
  fluid_settings_t *settings = nullptr;
  fluid_synth_t *synth = nullptr;
  fluid_audio_driver_t *audio_driver = nullptr;
  int font_id = -1;
  std::vector<std::string> font_paths;
  std::vector<DeviceSource> font_sources;
  std::size_t font_index = 0;

  bool InitAudio() {
    settings = new_fluid_settings();
    if (!settings) {
      return false;
    }
    fluid_settings_setnum(settings, "synth.sample-rate", kSampleRate);
    fluid_settings_setint(settings, "synth.audio-channels", 2);
    fluid_settings_setnum(settings, "synth.gain", 1.0);

    synth = new_fluid_synth(settings);
    if (!synth) {
      CleanupAudio();
      return false;
    }

    audio_driver = new_fluid_audio_driver(settings, synth);
#if !defined(WIN32)
    if (!audio_driver) {
      fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
      audio_driver = new_fluid_audio_driver(settings, synth);
    }
#endif
    if (!audio_driver) {
      CleanupAudio();
      return false;
    }
    return true;
  }

  void CleanupAudio() {
    if (audio_driver) {
      delete_fluid_audio_driver(audio_driver);
      audio_driver = nullptr;
    }
    if (synth) {
      if (font_id >= 0) {
        fluid_synth_sfunload(synth, font_id, 1);
        font_id = -1;
      }
      delete_fluid_synth(synth);
      synth = nullptr;
    }
    if (settings) {
      delete_fluid_settings(settings);
      settings = nullptr;
    }
  }

  void ScanSoundFonts(std::string_view data_path) {
    font_paths.clear();
    font_sources.clear();

#ifdef WIN32
    ScanDirectory("C:/Windows/system32/drivers", font_paths, font_sources,
                  DeviceSource::System);
#else
    ScanDirectory("/usr/share/sounds/sf2", font_paths, font_sources,
                  DeviceSource::System);
    ScanDirectory("/usr/share/soundfonts", font_paths, font_sources,
                  DeviceSource::System);
#endif

#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    if (const char *environment = std::getenv("DEFAULT_SOUNDFONT")) {
      font_paths.emplace_back(environment);
      font_sources.push_back(DeviceSource::Environment);
    }
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic pop
#endif

    ScanDirectory(std::string{data_path} + "soundfonts", font_paths,
                  font_sources, DeviceSource::Local);

    struct IndexedFont {
      const std::string *path;
      DeviceSource source;
    };
    std::vector<IndexedFont> indexed;
    indexed.reserve(font_paths.size());
    for (std::size_t i = 0; i < font_paths.size(); i++) {
      indexed.push_back({&font_paths[i], font_sources[i]});
    }
    std::ranges::sort(indexed, [](const auto &a, const auto &b) {
      return *a.path < *b.path;
    });
    const auto [first, last] =
        std::ranges::unique(indexed, std::ranges::equal_to{},
                            [](const auto &x) { return *x.path; });
    indexed.erase(first, last);

    std::vector<std::string> new_paths;
    std::vector<DeviceSource> new_sources;
    new_paths.reserve(indexed.size());
    new_sources.reserve(indexed.size());
    for (const auto &entry : indexed) {
      new_paths.push_back(*entry.path);
      new_sources.push_back(entry.source);
    }
    font_paths = std::move(new_paths);
    font_sources = std::move(new_sources);
  }

  std::size_t FindSoundFont(std::string_view name) const {
    if (name.empty()) {
      return font_paths.size();
    }
    const auto wanted = Basename(name);
    for (std::size_t i = 0; i < font_paths.size(); i++) {
      if (Basename(font_paths[i]) == wanted) {
        return i;
      }
    }
    return font_paths.size();
  }
};

MidiSynth::MidiSynth() : impl_(std::make_unique<Impl>()) {}
MidiSynth::~MidiSynth() { Shutdown(); }

AudioResult MidiSynth::Initialize(std::string_view data_path,
                                  std::string_view preferred_soundfont) {
  std::scoped_lock lock(impl_->mutex);
  if (impl_->synth) {
    return AudioResult::Fail(AudioError::AlreadyInitialized,
                             "MIDI synth is already initialized");
  }

  impl_->ScanSoundFonts(data_path);
  if (impl_->font_paths.empty()) {
    return AudioResult::Fail(AudioError::SoundFontLoadFailed,
                             "No SoundFont files were found");
  }

  impl_->font_index = impl_->FindSoundFont(preferred_soundfont);
  if (impl_->font_index == impl_->font_paths.size()) {
    impl_->font_index = 0;
  }

  if (!impl_->InitAudio()) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize FluidSynth audio");
  }

  impl_->font_id = fluid_synth_sfload(
      impl_->synth, impl_->font_paths[impl_->font_index].c_str(), 1);
  if (impl_->font_id == FLUID_FAILED) {
    const auto path = impl_->font_paths[impl_->font_index];
    impl_->CleanupAudio();
    return AudioResult::Fail(AudioError::SoundFontLoadFailed,
                             "Failed to load SoundFont: " + path);
  }

  logging::Info(logging::Channel::Audio, "Using SoundFont: {}",
                impl_->font_paths[impl_->font_index]);
  return AudioResult::Ok();
}

void MidiSynth::Shutdown() {
  std::scoped_lock lock(impl_->mutex);
  impl_->CleanupAudio();
  impl_->font_paths.clear();
  impl_->font_sources.clear();
  impl_->font_index = 0;
}

std::size_t MidiSynth::DeviceCount() const {
  std::scoped_lock lock(impl_->mutex);
  return impl_->font_paths.size();
}

std::optional<std::string> MidiSynth::DeviceName(std::size_t index) const {
  std::scoped_lock lock(impl_->mutex);
  if (index >= impl_->font_paths.size()) {
    return std::nullopt;
  }
  return Basename(impl_->font_paths[index]);
}

std::optional<DeviceSource> MidiSynth::DeviceSourceAt(std::size_t index) const {
  std::scoped_lock lock(impl_->mutex);
  if (index >= impl_->font_sources.size()) {
    return std::nullopt;
  }
  return impl_->font_sources[index];
}

std::optional<std::string> MidiSynth::CurrentDeviceName() const {
  std::scoped_lock lock(impl_->mutex);
  if (!impl_->synth || impl_->font_index >= impl_->font_paths.size()) {
    return std::nullopt;
  }
  return Basename(impl_->font_paths[impl_->font_index]);
}

bool MidiSynth::IsInitialized() const {
  std::scoped_lock lock(impl_->mutex);
  return impl_->synth != nullptr;
}

AudioResult MidiSynth::SelectDevice(std::size_t index) {
  std::scoped_lock lock(impl_->mutex);
  if (!impl_->synth) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "MIDI synth is not initialized");
  }
  if (index >= impl_->font_paths.size()) {
    return AudioResult::Fail(AudioError::InvalidArgument,
                             "SoundFont index out of range");
  }
  if (index == impl_->font_index) {
    return AudioResult::Ok();
  }

  const auto old_index = impl_->font_index;
  const auto old_font = impl_->font_paths[old_index];
  impl_->font_index = index;

  if (impl_->audio_driver) {
    delete_fluid_audio_driver(impl_->audio_driver);
    impl_->audio_driver = nullptr;
  }
  fluid_synth_sfunload(impl_->synth, impl_->font_id, 1);
  impl_->font_id = -1;

  impl_->font_id = fluid_synth_sfload(
      impl_->synth, impl_->font_paths[impl_->font_index].c_str(), 1);
  if (impl_->font_id == FLUID_FAILED) {
    impl_->font_index = old_index;
    impl_->font_id = fluid_synth_sfload(impl_->synth, old_font.c_str(), 1);
    if (impl_->font_id == FLUID_FAILED) {
      impl_->CleanupAudio();
      return AudioResult::Fail(
          AudioError::SoundFontRollbackFailed,
          "Failed to load the new SoundFont and rollback failed");
    }
  }

  impl_->audio_driver = new_fluid_audio_driver(impl_->settings, impl_->synth);
  if (!impl_->audio_driver) {
    impl_->CleanupAudio();
    impl_->font_index = old_index;
    if (impl_->InitAudio()) {
      impl_->font_id = fluid_synth_sfload(
          impl_->synth, impl_->font_paths[impl_->font_index].c_str(), 1);
      impl_->audio_driver =
          new_fluid_audio_driver(impl_->settings, impl_->synth);
    }
    return AudioResult::Fail(AudioError::DeviceSwitchFailed,
                             "Failed to restart the FluidSynth audio driver");
  }
  return AudioResult::Ok();
}

AudioResult MidiSynth::ChangeDevice(int direction) {
  std::scoped_lock lock(impl_->mutex);
  if (impl_->font_paths.size() <= 1) {
    return AudioResult::Ok();
  }
  const auto old_index = impl_->font_index;
  std::size_t new_index = 0;
  if (direction > 0) {
    new_index = ((impl_->font_index + 1) % impl_->font_paths.size());
  } else {
    new_index = ((impl_->font_index + impl_->font_paths.size() - 1) %
                 impl_->font_paths.size());
  }
  impl_->font_index = new_index;

  if (impl_->audio_driver) {
    delete_fluid_audio_driver(impl_->audio_driver);
    impl_->audio_driver = nullptr;
  }
  fluid_synth_sfunload(impl_->synth, impl_->font_id, 1);
  impl_->font_id = -1;

  impl_->font_id = fluid_synth_sfload(
      impl_->synth, impl_->font_paths[impl_->font_index].c_str(), 1);
  if (impl_->font_id == FLUID_FAILED) {
    impl_->font_index = old_index;
    impl_->font_id = fluid_synth_sfload(
        impl_->synth, impl_->font_paths[old_index].c_str(), 1);
    if (impl_->font_id == FLUID_FAILED) {
      impl_->CleanupAudio();
      return AudioResult::Fail(
          AudioError::SoundFontRollbackFailed,
          "Failed to switch SoundFont and rollback failed");
    }
  }

  impl_->audio_driver = new_fluid_audio_driver(impl_->settings, impl_->synth);
  if (!impl_->audio_driver) {
    impl_->CleanupAudio();
    impl_->font_index = old_index;
    if (impl_->InitAudio()) {
      impl_->font_id = fluid_synth_sfload(
          impl_->synth, impl_->font_paths[impl_->font_index].c_str(), 1);
      impl_->audio_driver =
          new_fluid_audio_driver(impl_->settings, impl_->synth);
    }
    return AudioResult::Fail(AudioError::DeviceSwitchFailed,
                             "Failed to restart the FluidSynth audio driver");
  }
  return AudioResult::Ok();
}

void MidiSynth::Output(std::uint8_t status, std::uint8_t a, std::uint8_t b) {
  std::scoped_lock lock(impl_->mutex);
  if (!impl_->synth) {
    return;
  }

  const int channel = (status & 0x0f);
  switch (status & 0xf0) {
  case 0x80:
    fluid_synth_noteoff(impl_->synth, channel, a);
    break;
  case 0x90:
    if (b == 0) {
      fluid_synth_noteoff(impl_->synth, channel, a);
    } else {
      fluid_synth_noteon(impl_->synth, channel, a, b);
    }
    break;
  case 0xa0:
    fluid_synth_key_pressure(impl_->synth, channel, a, b);
    break;
  case 0xb0:
    fluid_synth_cc(impl_->synth, channel, a, b);
    break;
  case 0xc0:
    fluid_synth_program_change(impl_->synth, channel, a);
    break;
  case 0xd0:
    fluid_synth_channel_pressure(impl_->synth, channel, a);
    break;
  case 0xe0:
    fluid_synth_pitch_bend(impl_->synth, channel, (a | (b << 7)));
    break;
  default:
    break;
  }
}

void MidiSynth::OutputSysEx(std::span<const std::uint8_t> message) {
  std::scoped_lock lock(impl_->mutex);
  if (!impl_->synth || message.empty()) {
    return;
  }
  const auto *data = reinterpret_cast<const char *>(message.data() + 1);
  fluid_synth_sysex(impl_->synth, data, static_cast<int>(message.size() - 1),
                    nullptr, nullptr, nullptr, 0);
}

void MidiSynth::Panic() {
  std::scoped_lock lock(impl_->mutex);
  if (impl_->synth) {
    fluid_synth_all_sounds_off(impl_->synth, -1);
  }
}

void MidiSynth::Pause() {
  std::scoped_lock lock(impl_->mutex);
  if (impl_->audio_driver) {
    delete_fluid_audio_driver(impl_->audio_driver);
    impl_->audio_driver = nullptr;
  }
}

void MidiSynth::Resume() {
  std::scoped_lock lock(impl_->mutex);
  if (!impl_->synth || impl_->audio_driver) {
    return;
  }
  impl_->audio_driver = new_fluid_audio_driver(impl_->settings, impl_->synth);
  if (!impl_->audio_driver) {
    logging::Error(logging::Channel::Audio,
                   "Failed to resume FluidSynth audio driver");
  }
}

} // namespace audio::bgm
