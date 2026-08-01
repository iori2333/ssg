#include "audio_system.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <span>
#include <string_view>
#include <thread>
#include <utility>

#include <SDL3/SDL_audio.h>

#include "audio/core/audio_types.h"

namespace audio {
namespace {

using namespace std::chrono_literals;

float LinearVolume(Volume volume) {
  return (static_cast<float>(volume) /
          static_cast<float>(kMaxVolume));
}

} // namespace

AudioSystem::AudioSystem()
    : waveform_(engine_.Get()), sfx_(engine_.Get()),
      bgm_(waveform_, sequencer_, synth_) {}

AudioSystem::~AudioSystem() { Shutdown(); }

AudioResult AudioSystem::Initialize(std::string_view data_path,
                                    std::string_view preferred_soundfont) {
  if (initialized_) {
    return AudioResult::Fail(AudioError::AlreadyInitialized,
                             "Audio system is already initialized");
  }

  auto result = engine_.Initialize();
  if (!result.success) {
    return result;
  }
  result = sfx_.Initialize();
  if (!result.success) {
    engine_.Shutdown();
    return result;
  }
  sfx_initialized_ = true;

  const auto midi_result = synth_.Initialize(data_path, preferred_soundfont);
  midi_available_ = midi_result.success;
  initialized_ = true;
  StartTimer();
  return AudioResult::Ok();
}

void AudioSystem::Shutdown() {
  if (!initialized_) {
    return;
  }
  StopTimer();
  bgm_.Stop();
  synth_.Shutdown();
  sfx_.Shutdown();
  engine_.Shutdown();
  sfx_initialized_ = false;
  midi_available_ = false;
  initialized_ = false;
}

bool AudioSystem::IsEnabled() const { return initialized_; }

bool AudioSystem::IsMidiAvailable() const { return midi_available_; }

AudioResult AudioSystem::LoadBgmMidi(midi::SequenceData sequence) {
  if (!initialized_) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "Audio system is not initialized");
  }
  return bgm_.LoadMidi(std::move(sequence));
}

AudioResult AudioSystem::LoadBgmWaveform(std::string_view path) {
  if (!initialized_) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "Audio system is not initialized");
  }
  return bgm_.LoadWaveform(path);
}

void AudioSystem::ClearBgmWaveform() {
  if (initialized_) {
    bgm_.ClearWaveform();
  }
}

void AudioSystem::PlayBgm() {
  if (initialized_) {
    bgm_.Play();
  }
}

void AudioSystem::StopBgm() {
  if (initialized_) {
    bgm_.Stop();
  }
}

void AudioSystem::PauseBgm() {
  if (initialized_) {
    bgm_.Pause();
  }
}

void AudioSystem::ResumeBgm() {
  if (initialized_) {
    bgm_.Resume();
  }
}

void AudioSystem::FadeOutBgm(std::uint8_t speed) {
  if (!initialized_) {
    return;
  }
  const auto volume_start =
      (bgm_volume_ == 0) ? 0 : static_cast<Volume>(bgm_volume_ - 1);
  const auto duration =
      (10ms * kMaxVolume *
       ((((256 - speed) * 4) / (kMaxVolume + 1)) + 1));
  bgm_.FadeOut(LinearVolume(volume_start), duration);
}

void AudioSystem::SetBgmVolume(Volume volume) {
  bgm_volume_ = std::min(volume, kMaxVolume);
  if (initialized_) {
    bgm_.SetVolume(bgm_volume_);
  }
}

void AudioSystem::SetSfxVolume(Volume volume) {
  sfx_volume_ = std::min(volume, kMaxVolume);
  if (sfx_initialized_) {
    sfx_.SetVolume(LinearVolume(sfx_volume_));
  }
}

void AudioSystem::SetBgmTempo(std::int8_t tempo) {
  if (initialized_) {
    bgm_.SetTempo(tempo);
  }
}

void AudioSystem::SetBgmGainApplied(bool enabled) {
  if (initialized_) {
    bgm_.SetGainApplied(enabled);
  }
}

BgmSnapshot AudioSystem::BgmSnapshot() const {
  return bgm_.Snapshot();
}

midi::Visualization AudioSystem::MidiVisualization() const {
  return sequencer_.Snapshot();
}

std::size_t AudioSystem::MidiDeviceCount() const {
  return midi_available_ ? synth_.DeviceCount() : 0;
}

std::optional<std::string>
AudioSystem::MidiDeviceNameAt(std::size_t index) const {
  return midi_available_ ? synth_.DeviceName(index) : std::nullopt;
}

std::optional<midi::DeviceSource>
AudioSystem::MidiDeviceSourceAt(std::size_t index) const {
  return midi_available_ ? synth_.DeviceSourceAt(index) : std::nullopt;
}

std::optional<std::string> AudioSystem::MidiCurrentDeviceName() const {
  return midi_available_ ? synth_.CurrentDeviceName() : std::nullopt;
}

AudioResult AudioSystem::SelectMidiDevice(std::size_t index) {
  return midi_available_ ? synth_.SelectDevice(index)
                         : AudioResult::Fail(AudioError::NotEnabled,
                                             "MIDI is not available");
}

AudioResult AudioSystem::ChangeMidiDevice(int direction) {
  return midi_available_ ? synth_.ChangeDevice(direction)
                         : AudioResult::Fail(AudioError::NotEnabled,
                                             "MIDI is not available");
}

void AudioSystem::SetMidiFixSysExBugs(bool enabled) {
  sequencer_.SetFixSysExBugs(enabled);
}

AudioResult AudioSystem::LoadSfx(std::uint8_t id,
                                 const SDL_AudioSpec &spec,
                                 std::span<const std::uint8_t> pcm,
                                 std::uint8_t max_instances) {
  if (!sfx_initialized_) {
    return AudioResult::Fail(AudioError::NotInitialized,
                             "Sound effects are not initialized");
  }
  return sfx_.Load(id, spec, pcm, max_instances);
}

void AudioSystem::PlaySfx(std::uint8_t id, float pan, bool loop) {
  if (sfx_initialized_) {
    sfx_.Play(id, pan, loop);
  }
}

void AudioSystem::StopSfx(std::uint8_t id) {
  if (sfx_initialized_) {
    sfx_.Stop(id);
  }
}

void AudioSystem::StopAllSfx() {
  if (sfx_initialized_) {
    sfx_.StopAll();
  }
}

void AudioSystem::PauseAll() {
  if (initialized_) {
    bgm_.Pause();
    sfx_.PauseAll();
  }
}

void AudioSystem::ResumeAll() {
  if (initialized_) {
    sfx_.ResumeAll();
    bgm_.Resume();
  }
}

void AudioSystem::StartTimer() {
  if (timer_.joinable()) {
    return;
  }
  timer_ = std::jthread([this](std::stop_token stop) {
    auto next_tick = std::chrono::steady_clock::now();
    while (!stop.stop_requested()) {
      next_tick += 10ms;
      std::this_thread::sleep_until(next_tick);
      if (!stop.stop_requested()) {
        bgm_.Tick(10ms);
      }
    }
  });
}

void AudioSystem::StopTimer() {
  if (timer_.joinable()) {
    timer_.request_stop();
    timer_.join();
  }
}

} // namespace audio
