#include "waveform_player.h"

#include <chrono>
#include <string_view>

#include <miniaudio.h>

#include "audio/core/audio_types.h"

namespace audio::stream {

WaveformPlayer::WaveformPlayer(ma_engine &engine) : engine_(engine) {}

WaveformPlayer::~WaveformPlayer() { Unload(); }

AudioResult WaveformPlayer::Load(std::string_view path) {
  Unload();

  auto source = std::make_unique<WaveformSource>();
  auto result = source->Load(path);
  if (!result.success) {
    return result;
  }

  if (ma_sound_init_from_data_source(
          &engine_, source->DataSource(), MA_SOUND_FLAG_NO_SPATIALIZATION,
          nullptr, &sound_) != MA_SUCCESS) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize waveform sound");
  }
  source_ = std::move(source);
  sound_initialized_ = true;
  return AudioResult::Ok();
}

void WaveformPlayer::Unload() {
  if (sound_initialized_) {
    ma_sound_uninit(&sound_);
    sound_initialized_ = false;
  }
  source_.reset();
}

void WaveformPlayer::Play() {
  if (IsLoaded()) {
    ma_sound_start(&sound_);
  }
}

void WaveformPlayer::Stop() {
  if (IsLoaded()) {
    ma_sound_stop(&sound_);
  }
}

void WaveformPlayer::Pause() {
  if (IsLoaded()) {
    ma_sound_stop(&sound_);
  }
}

void WaveformPlayer::Resume() {
  if (IsLoaded()) {
    ma_sound_start(&sound_);
  }
}

void WaveformPlayer::SetVolume(float linear) {
  if (IsLoaded()) {
    ma_sound_set_volume(&sound_, linear);
  }
}

void WaveformPlayer::SetPitch(float factor) {
  if (IsLoaded()) {
    ma_sound_set_pitch(&sound_, factor);
  }
}

void WaveformPlayer::FadeOut(float from,
                             std::chrono::milliseconds duration) {
  if (IsLoaded()) {
    source_->Track().FadeOut(from, duration);
  }
}

bool WaveformPlayer::IsLoaded() const {
  return sound_initialized_ && source_ && source_->IsLoaded();
}

std::string_view WaveformPlayer::Title() const {
  return source_ ? source_->Title() : std::string_view{};
}

std::chrono::milliseconds WaveformPlayer::PlayTime() const {
  if (!IsLoaded()) {
    return std::chrono::milliseconds::zero();
  }
  return std::chrono::milliseconds{
      ma_sound_get_time_in_milliseconds(&sound_)};
}

::bgm::Track &WaveformPlayer::Track() { return source_->Track(); }

} // namespace audio::stream
