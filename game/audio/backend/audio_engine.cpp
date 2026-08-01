#include "audio_engine.h"

#include <miniaudio.h>

namespace audio::backend {

AudioEngine::~AudioEngine() { Shutdown(); }

AudioResult AudioEngine::Initialize() {
  if (initialized_) {
    return AudioResult::Fail(AudioError::AlreadyInitialized,
                             "Audio engine is already initialized");
  }
  if (ma_engine_init(nullptr, &engine_) != MA_SUCCESS) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize miniaudio");
  }
  initialized_ = true;
  return AudioResult::Ok();
}

void AudioEngine::Shutdown() {
  if (initialized_) {
    ma_engine_uninit(&engine_);
    initialized_ = false;
  }
}

ma_engine &AudioEngine::Get() { return engine_; }

bool AudioEngine::IsInitialized() const { return initialized_; }

} // namespace audio::backend

