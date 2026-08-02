#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include "bgm_controller.h"
#include "midi/midi_parser.h"
#include "midi/midi_sequencer.h"
#include "midi/midi_synth.h"
#include "midi_track.h"
#include "pcm_track.h"

#include "audio/core/audio_types.h"

namespace audio::bgm {

BgmController::BgmController(ma_engine &engine, MidiSequencer &sequencer,
                             MidiSynth &synth)
    : engine_(engine), sequencer_(sequencer), synth_(synth) {}

AudioResult BgmController::LoadMidi(SequenceData sequence) {
  if (active_ != nullptr) {
    active_->Stop();
  }
  playing_.store(false);
  waveform_.reset();
  midi_.reset();

  auto midi = std::make_unique<MidiTrack>(sequencer_, synth_);
  midi->Load(std::move(sequence));
  ApplyTrackSettings(*midi);
  midi_ = std::move(midi);
  active_ = midi_.get();
  state_ = PlaybackState::Ready;
  return AudioResult::Ok();
}

AudioResult BgmController::LoadWaveform(std::string_view path) {
  auto track = std::make_unique<PcmTrack>(engine_);
  auto result = track->Load(path);
  if (!result.success) {
    return result;
  }

  if (active_ != nullptr) {
    active_->Stop();
  }
  playing_.store(false);
  waveform_.reset();
  ApplyTrackSettings(*track);
  waveform_ = std::move(track);
  active_ = waveform_.get();
  state_ = PlaybackState::Ready;
  return AudioResult::Ok();
}

void BgmController::ClearWaveform() {
  if (active_ != nullptr) {
    active_->Stop();
  }
  playing_.store(false);
  waveform_.reset();
  active_ = midi_ ? midi_.get() : nullptr;
  state_ = ((active_ != nullptr) ? PlaybackState::Ready : PlaybackState::Idle);
}

void BgmController::Play() {
  if ((active_ == nullptr) || !active_->IsLoaded() ||
      state_ == PlaybackState::Faulted) {
    return;
  }
  active_->Play();
  playing_.store(true);
  state_ = PlaybackState::Playing;
}

void BgmController::Stop() {
  playing_.store(false);
  if (active_ != nullptr) {
    active_->Stop();
  }
  state_ =
      (((active_ != nullptr) && active_->IsLoaded()) ? PlaybackState::Ready
                                                     : PlaybackState::Idle);
}

void BgmController::Pause() {
  if (!playing_.load()) {
    return;
  }
  if (active_ != nullptr) {
    active_->Pause();
  }
  playing_.store(false);
  state_ = PlaybackState::Paused;
}

void BgmController::Resume() {
  if (playing_.load() || (active_ == nullptr) || !active_->IsLoaded()) {
    return;
  }
  active_->Resume();
  playing_.store(true);
  state_ = PlaybackState::Playing;
}

void BgmController::FadeOut(float volume_start,
                            std::chrono::milliseconds duration) {
  if (active_ != nullptr) {
    active_->FadeOut(volume_start, duration);
  }
}

void BgmController::SetVolume(Volume volume) {
  volume_ = std::min(volume, kMaxVolume);
  if (active_ != nullptr) {
    active_->SetVolume(volume_);
  }
  if (midi_ && active_ != midi_.get()) {
    midi_->SetVolume(volume_);
  }
}

void BgmController::SetTempo(std::int8_t tempo) {
  tempo_ = std::clamp(tempo, std::int8_t{-100}, std::int8_t{100});
  if (active_ != nullptr) {
    active_->SetTempo(tempo_);
  }
  if (midi_ && active_ != midi_.get()) {
    midi_->SetTempo(tempo_);
  }
}

void BgmController::Tick(std::chrono::milliseconds delta) {
  if (!playing_.load() || (active_ == nullptr)) {
    return;
  }
  active_->Tick(delta);
  if (active_->IsPlaying() && midi_ && active_ != midi_.get()) {
    midi_->TickBackground(delta);
  }
  if (!active_->IsPlaying()) {
    playing_.store(false);
    state_ = (active_->IsLoaded() ? PlaybackState::Ready : PlaybackState::Idle);
  }
}

BgmSnapshot BgmController::Snapshot() const {
  BgmSnapshot snapshot;
  snapshot.state = state_;
  snapshot.tempo = tempo_;
  if (active_ == nullptr) {
    return snapshot;
  }
  snapshot.mode = active_->Mode();
  snapshot.play_time = active_->PlayTime();
  return snapshot;
}

bool BgmController::IsLoaded() const {
  return (active_ != nullptr) && active_->IsLoaded();
}

void BgmController::ApplyTrackSettings(Track &track) const {
  track.SetVolume(volume_);
  track.SetTempo(tempo_);
}

} // namespace audio::bgm
