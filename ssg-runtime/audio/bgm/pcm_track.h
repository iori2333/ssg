/// PCM BGM track backed by miniaudio and decoded PCM sources.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

#include <miniaudio.h>

#include "track.h"

namespace audio::bgm {

class PcmTrack final : public Track {
public:
  class Source;

  explicit PcmTrack(ma_engine &engine);
  ~PcmTrack() override;
  PcmTrack(const PcmTrack &) = delete;
  PcmTrack &operator=(const PcmTrack &) = delete;
  PcmTrack(PcmTrack &&) = delete;
  PcmTrack &operator=(PcmTrack &&) = delete;

  AudioResult Load(std::string_view path);
  void Unload();

  void Play() override;
  void Stop() override;
  void Pause() override;
  void Resume() override;
  void FadeOut(float volume_start, std::chrono::milliseconds duration) override;
  void SetVolume(Volume volume) override;
  void SetTempo(int tempo) override;
  void Tick(std::chrono::milliseconds delta) override;

  [[nodiscard]] bool IsLoaded() const override;
  [[nodiscard]] bool IsPlaying() const override;
  [[nodiscard]] BgmMode Mode() const override;
  [[nodiscard]] std::chrono::milliseconds PlayTime() const override;
  [[nodiscard]] float FadeVolumeLinear() const override;

private:
  void ApplyVolume();

  ma_engine &engine_;
  std::unique_ptr<Source> source_;
  ma_sound sound_{};
  bool sound_initialized_ = false;
  std::atomic<bool> playing_ = false;
  Volume volume_ = kMaxVolume;
  int tempo_ = 0;
};

} // namespace audio::bgm
