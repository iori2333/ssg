#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <miniaudio.h>

#include "audio_system.h"
#include "bgm/bgm_controller.h"
#include "bgm/midi/midi_parser.h"
#include "bgm/midi/midi_sequencer.h"
#include "bgm/midi/midi_synth.h"
#include "core/audio_types.h"
#include "sfx.h"
#include "sfx/sfx_bank.h"

#include "util/guard.h"

namespace audio {
namespace {

using namespace std::chrono_literals;

float LinearVolume(Volume volume) {
  return (static_cast<float>(volume) / static_cast<float>(kMaxVolume));
}

} // namespace

struct AudioSystem::Impl {
  ma_engine engine{};
  sfx::SfxBank sfx{engine};
  bgm::MidiSequencer sequencer;
  bgm::MidiSynth synth;
  bgm::BgmController bgm{engine, sequencer, synth};
  std::jthread timer;
  std::string data_path;

  bool engine_initialized = false;
  bool initialized = false;
  bool midi_available = false;
  bool sfx_initialized = false;
  Volume bgm_volume = kMaxVolume;
  Volume sfx_volume = kMaxVolume;

  void StartTimer() {
    if (timer.joinable()) {
      return;
    }
    timer = std::jthread([this](const std::stop_token &stop) {
      auto next_tick = std::chrono::steady_clock::now();
      while (!stop.stop_requested()) {
        next_tick += 10ms;
        std::this_thread::sleep_until(next_tick);
        if (!stop.stop_requested()) {
          bgm.Tick(10ms);
        }
      }
    });
  }

  void StopTimer() {
    if (timer.joinable()) {
      timer.request_stop();
      timer.join();
    }
  }
};

AudioSystem::AudioSystem() : impl_(std::make_unique<Impl>()) {}

AudioSystem::~AudioSystem() { Shutdown(); }

AudioResult AudioSystem::Initialize(std::string_view data_path,
                                    std::string_view preferred_soundfont) {
  if (impl_->initialized || impl_->engine_initialized) {
    return AudioResult::Fail(AudioError::AlreadyInitialized,
                             "Audio system is already initialized");
  }

  if (ma_engine_init(nullptr, &impl_->engine) != MA_SUCCESS) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize miniaudio");
  }
  impl_->engine_initialized = true;

  impl_->data_path = std::string{data_path};
  auto result = impl_->sfx.Initialize();
  if (!result.success) {
    ma_engine_uninit(&impl_->engine);
    impl_->engine_initialized = false;
    return result;
  }
  impl_->sfx_initialized = true;

  const auto midi_result =
      impl_->synth.Initialize(data_path, preferred_soundfont);
  impl_->midi_available = midi_result.success;
  impl_->initialized = true;
  impl_->StartTimer();
  return AudioResult::Ok();
}

void AudioSystem::Shutdown() {
  if (!impl_->initialized) {
    return;
  }
  impl_->StopTimer();
  impl_->bgm.Stop();
  impl_->synth.Shutdown();
  impl_->sfx.Shutdown();
  ma_engine_uninit(&impl_->engine);
  impl_->engine_initialized = false;
  impl_->sfx_initialized = false;
  impl_->midi_available = false;
  impl_->initialized = false;
}

bool AudioSystem::IsEnabled() const { return impl_->initialized; }

bool AudioSystem::IsMidiAvailable() const { return impl_->midi_available; }

bool AudioSystem::EnableBgm(bool enabled, std::string_view soundfont) {
  if (!enabled) {
    StopBgm();
    return true;
  }
  if (impl_->initialized) {
    return true;
  }
  return Initialize(impl_->data_path, soundfont).success;
}

void AudioSystem::SetVolumes(Volume bgm, Volume sfx) {
  SetBgmVolume(bgm);
  SetSfxVolume(sfx);
}

AudioResult AudioSystem::LoadBgmMidi(bgm::SequenceData sequence) {
  if (!impl_->initialized) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "Audio system is not initialized");
  }
  return impl_->bgm.LoadMidi(std::move(sequence));
}

AudioResult AudioSystem::LoadBgmWaveform(std::string_view path) {
  if (!impl_->initialized) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "Audio system is not initialized");
  }
  return impl_->bgm.LoadWaveform(path);
}

void AudioSystem::ClearBgmWaveform() {
  if (impl_->initialized) {
    impl_->bgm.ClearWaveform();
  }
}

void AudioSystem::PlayBgm() {
  if (impl_->initialized) {
    impl_->bgm.Play();
  }
}

void AudioSystem::StopBgm() {
  if (impl_->initialized) {
    impl_->bgm.Stop();
  }
}

void AudioSystem::PauseBgm() {
  if (impl_->initialized) {
    impl_->bgm.Pause();
  }
}

void AudioSystem::ResumeBgm() {
  if (impl_->initialized) {
    impl_->bgm.Resume();
  }
}

void AudioSystem::FadeOutBgm(int speed) {
  if (!impl_->initialized) {
    return;
  }
  const auto volume_start =
      (impl_->bgm_volume == 0) ? 0 : static_cast<Volume>(impl_->bgm_volume - 1);
  const auto duration =
      (10ms * kMaxVolume * ((((256 - speed) * 4) / (kMaxVolume + 1)) + 1));
  impl_->bgm.FadeOut(LinearVolume(volume_start), duration);
}

void AudioSystem::SetBgmVolume(Volume volume) {
  impl_->bgm_volume = std::min(volume, kMaxVolume);
  if (impl_->initialized) {
    impl_->bgm.SetVolume(impl_->bgm_volume);
  }
}

void AudioSystem::SetSfxVolume(Volume volume) {
  impl_->sfx_volume = std::min(volume, kMaxVolume);
  if (impl_->sfx_initialized) {
    impl_->sfx.SetVolume(LinearVolume(impl_->sfx_volume));
  }
}

void AudioSystem::SetBgmTempo(int tempo) {
  if (impl_->initialized) {
    impl_->bgm.SetTempo(tempo);
  }
}

BgmSnapshot AudioSystem::BgmSnapshot() const { return impl_->bgm.Snapshot(); }

bgm::Visualization AudioSystem::MidiVisualization() const {
  return impl_->sequencer.Snapshot();
}

std::size_t AudioSystem::MidiDeviceCount() const {
  return impl_->midi_available ? impl_->synth.DeviceCount() : 0;
}

std::optional<std::string>
AudioSystem::MidiDeviceNameAt(std::size_t index) const {
  return impl_->midi_available ? impl_->synth.DeviceName(index) : std::nullopt;
}

std::optional<bgm::DeviceSource>
AudioSystem::MidiDeviceSourceAt(std::size_t index) const {
  return impl_->midi_available ? impl_->synth.DeviceSourceAt(index)
                               : std::nullopt;
}

std::optional<std::string> AudioSystem::MidiCurrentDeviceName() const {
  return impl_->midi_available ? impl_->synth.CurrentDeviceName()
                               : std::nullopt;
}

AudioResult AudioSystem::SelectMidiDevice(std::size_t index) {
  return impl_->midi_available ? impl_->synth.SelectDevice(index)
                               : AudioResult::Fail(AudioError::NotEnabled,
                                                   "MIDI is not available");
}

AudioResult AudioSystem::ChangeMidiDevice(int direction) {
  return impl_->midi_available ? impl_->synth.ChangeDevice(direction)
                               : AudioResult::Fail(AudioError::NotEnabled,
                                                   "MIDI is not available");
}

void AudioSystem::SetMidiFixSysExBugs(bool enabled) {
  impl_->sequencer.SetFixSysExBugs(enabled);
}

AudioResult AudioSystem::LoadSfx(SfxId id,
                                 std::span<const std::uint8_t> wav,
                                 int max_instances) {
  if (!impl_->sfx_initialized) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "Sound effects are not initialized");
  }
  if (wav.empty()) {
    return AudioResult::Fail(AudioError::InvalidArgument,
                             "Sound effect data is empty");
  }

  auto *io = SDL_IOFromConstMem(wav.data(), wav.size());
  if (io == nullptr) {
    return AudioResult::Fail(AudioError::DecodeFailed,
                             "Failed to open sound effect data");
  }

  SDL_AudioSpec spec{};
  std::uint8_t *pcm = nullptr;
  std::uint32_t pcm_len = 0;
  if (!SDL_LoadWAV_IO(io, true, &spec, &pcm, &pcm_len)) {
    return AudioResult::Fail(AudioError::DecodeFailed,
                             "Failed to decode sound effect WAV");
  }
  auto pcm_guard = util::MakeGuard(pcm, SDL_free);
  return impl_->sfx.Load(id, spec, {pcm, pcm_len}, max_instances);
}

void AudioSystem::PlaySfx(SfxId id, int x, bool loop) {
  if (impl_->sfx_initialized) {
    impl_->sfx.Play(id, SoundPanForWorldX(x), loop);
  }
}

void AudioSystem::StopSfx(SfxId id) {
  if (impl_->sfx_initialized) {
    impl_->sfx.Stop(id);
  }
}

void AudioSystem::StopAllSfx() {
  if (impl_->sfx_initialized) {
    impl_->sfx.StopAll();
  }
}

void AudioSystem::PauseAll() {
  if (impl_->initialized) {
    impl_->bgm.Pause();
    // Keep the engine running so menu SFX stay audible during paused states.
    impl_->sfx.StopAll();
  }
}

void AudioSystem::ResumeAll() {
  if (impl_->initialized) {
    impl_->bgm.Resume();
  }
}

} // namespace audio
