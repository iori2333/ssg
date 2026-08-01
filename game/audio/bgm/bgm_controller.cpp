#include "bgm_controller.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "midi_track.h"
#include "pcm_track.h"

#include "audio/core/audio_types.h"
#include "audio/midi/midi_sequencer.h"
#include "audio/midi/midi_synth.h"

namespace audio::bgm {

BgmController::BgmController(ma_engine &engine, midi::MidiSequencer &sequencer,
                             midi::MidiSynth &synth)
    : engine_(engine), sequencer_(sequencer), synth_(synth) {}

AudioResult BgmController::LoadMidi(midi::SequenceData sequence) {
  if (active_) {
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

  if (active_) {
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
  if (active_) {
    active_->Stop();
  }
  playing_.store(false);
  waveform_.reset();
  active_ = midi_ ? midi_.get() : nullptr;
  state_ = (active_ ? PlaybackState::Ready : PlaybackState::Idle);
}

void BgmController::Play() {
  if (!active_ || !active_->IsLoaded() || state_ == PlaybackState::Faulted) {
    return;
  }
  active_->Play();
  playing_.store(true);
  state_ = PlaybackState::Playing;
}

void BgmController::Stop() {
  playing_.store(false);
  if (active_) {
    active_->Stop();
  }
  state_ = ((active_ && active_->IsLoaded()) ? PlaybackState::Ready
                                             : PlaybackState::Idle);
}

void BgmController::Pause() {
  if (!playing_.load()) {
    return;
  }
  if (active_) {
    active_->Pause();
  }
  playing_.store(false);
  state_ = PlaybackState::Paused;
}

void BgmController::Resume() {
  if (playing_.load() || !active_ || !active_->IsLoaded()) {
    return;
  }
  active_->Resume();
  playing_.store(true);
  state_ = PlaybackState::Playing;
}

void BgmController::FadeOut(float volume_start,
                            std::chrono::milliseconds duration) {
  if (active_) {
    active_->FadeOut(volume_start, duration);
  }
}

void BgmController::SetVolume(Volume volume) {
  volume_ = std::min(volume, kMaxVolume);
  if (active_) {
    active_->SetVolume(volume_);
  }
  if (midi_ && active_ != midi_.get()) {
    midi_->SetVolume(volume_);
  }
}

void BgmController::SetTempo(std::int8_t tempo) {
  tempo_ = std::clamp(tempo, std::int8_t{-100}, std::int8_t{100});
  if (active_) {
    active_->SetTempo(tempo_);
  }
  if (midi_ && active_ != midi_.get()) {
    midi_->SetTempo(tempo_);
  }
}

void BgmController::SetGainApplied(bool enabled) {
  gain_applied_ = enabled;
  if (active_) {
    active_->SetGainApplied(enabled);
  }
}

void BgmController::Tick(std::chrono::milliseconds delta) {
  if (!playing_.load() || !active_) {
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
  snapshot.gain_applied = gain_applied_;
  if (!active_) {
    return snapshot;
  }
  snapshot.mode = active_->Mode();
  snapshot.title = std::string{active_->Title()};
  snapshot.play_time = active_->PlayTime();
  return snapshot;
}

bool BgmController::IsLoaded() const { return active_ && active_->IsLoaded(); }

void BgmController::ApplyTrackSettings(Track &track) {
  track.SetVolume(volume_);
  track.SetTempo(tempo_);
  track.SetGainApplied(gain_applied_);
}

} // namespace audio::bgm
