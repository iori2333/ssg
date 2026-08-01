///
/// MusicPlayer - track playback, metadata, and BGM pack selection
///
#include <filesystem>
#include <format>
#include <utility>

#include "music_player.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "sys/log.h"
#include "sys/path.h"

static constexpr std::string_view kBgmRoot = "bgm/";

// ---------------------------------------------------------------------------
// Track switching
// ---------------------------------------------------------------------------

bool MusicPlayer::Play(unsigned int id) {
  if (!BgmIsEnabled()) {
    return false;
  }
  BgmStop();
  BgmClearWaveform();

  // Always load MIDI for sequencer notes and fallback audio.
  auto midi = midi_variant_ == MidiVariant::Arranged
                  ? data_.ExtractArrangedMusicMidi(id)
                  : data_.ExtractMusicMidi(id);
  if (midi.empty() || !MidiLoad(std::move(midi))) {
    logging::Error(logging::Channel::Music,
                   "Failed to load MIDI for track {} variant={}", id,
                   std::to_underlying(midi_variant_));
    return false;
  }

  // Try to open a replacement waveform from the active BGM pack.
  if (!pack_path_.empty()) {
    auto path = std::format("{}{:02}", pack_path_, id + 1);
    if (!BgmLoadWaveform(path)) {
      logging::Debug(logging::Channel::Music,
                     "No waveform replacement for track {}; using MIDI", id);
    }
  }

  loaded_num_ = id + 1;
  BgmSetLoadedTrackNumber(loaded_num_);
  BgmSetTempo(BgmTempo());
  BgmPlay();
  logging::Debug(logging::Channel::Music, "Playing track {} variant={}", id,
                 std::to_underlying(midi_variant_));
  return true;
}

size_t MusicPlayer::TrackCount() const { return data_.TrackCount(); }

void MusicPlayer::SetMidiVariant(MidiVariant variant) {
  if (midi_variant_ == variant) {
    return;
  }
  midi_variant_ = variant;
  if (loaded_num_ != 0 && BgmPlayingSource() != BgmPlaybackSource::None) {
    Play(loaded_num_ - 1);
  }
}

// ---------------------------------------------------------------------------
// BGM pack management
// ---------------------------------------------------------------------------

bool MusicPlayer::HasPacks(bool invalidate_cache) {
  if (packs_available_.has_value() && !invalidate_cache) {
    return packs_available_.value();
  }
  std::error_code error;
  packs_available_ = false;
  for (const auto &entry :
       std::filesystem::directory_iterator{kBgmRoot, error}) {
    if (error) {
      break;
    }
    if (entry.is_directory(error)) {
      packs_available_ = true;
      break;
    }
  }
  return packs_available_.value();
}

void MusicPlayer::ForEachPack(std::function<void(std::string_view)> func) {
  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator{kBgmRoot, error}) {
    if (error) {
      break;
    }
    if (entry.is_directory(error)) {
      const auto name = entry.path().filename().string();
      func(name);
    }
  }
}

bool MusicPlayer::SetPack(std::string_view pack) {
  std::string_view cur = pack_path_;
  if (!pack.empty()) {
    const auto path_data = PathForData();
    const auto root_len = (path_data.size() + kBgmRoot.size());
    if ((cur.size() > root_len) &&
        (cur.substr(root_len, pack.size()) == pack) &&
        (cur[root_len + pack.size()] == '/')) {
      return true;
    }
    pack_path_ = std::format("{}{}{}/", path_data, kBgmRoot, pack);

    std::error_code error;
    if (!std::filesystem::is_directory(pack_path_, error)) {
      logging::Warning(logging::Channel::Music,
                       "BGM pack directory is unavailable: {}", pack_path_);
      pack_path_.clear();
      return false;
    }
  } else {
    if (cur.empty()) {
      return true;
    }
    pack_path_.clear();
  }

  if ((loaded_num_ != 0) && (BgmPlayingSource() != BgmPlaybackSource::None)) {
    Play(loaded_num_ - 1);
  }
  return true;
}
