///
/// MusicPlayer - track playback, metadata, and BGM pack selection
///
#include <filesystem>
#include <format>
#include <utility>

#include "music_player.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "sys/path.h"

static constexpr std::string_view BGM_ROOT = "bgm/";

// ---------------------------------------------------------------------------
// Track switching
// ---------------------------------------------------------------------------

bool MusicPlayer::Play(unsigned int id) {
  if (!BGM_Enabled()) {
    return false;
  }
  BGM_Stop();
  BGM_ClearWaveform();

  // Always load MIDI for sequencer notes and fallback audio.
  auto midi = midi_variant_ == MidiVariant::Arranged
                  ? data_.ExtractArrangedMusicMidi(id)
                  : data_.ExtractMusicMidi(id);
  if (midi.empty() || !Mid_Load(std::move(midi))) {
    return false;
  }

  // Try to open a replacement waveform from the active BGM pack.
  if (!pack_path_.empty()) {
    auto path = std::format("{}{:02}", pack_path_, id + 1);
    BGM_LoadWaveform(path); // may fail silently — MIDI remains loaded
  }

  loaded_num_ = id + 1;
  BGM_SetLoadedNum(loaded_num_);
  BGM_SetTempo(BGM_GetTempo());
  BGM_Play();
  return true;
}

size_t MusicPlayer::TrackCount() const { return data_.TrackCount(); }

void MusicPlayer::SetMidiVariant(MidiVariant variant) {
  if (midi_variant_ == variant) {
    return;
  }
  midi_variant_ = variant;
  if (loaded_num_ != 0 && BGM_Playing() != BGM_PLAYING::NONE) {
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
       std::filesystem::directory_iterator{BGM_ROOT, error}) {
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
       std::filesystem::directory_iterator{BGM_ROOT, error}) {
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
    const auto root_len = (path_data.size() + BGM_ROOT.size());
    if ((cur.size() > root_len) &&
        (cur.substr(root_len, pack.size()) == pack) &&
        (cur[root_len + pack.size()] == '/')) {
      return true;
    }
    pack_path_ = std::format("{}{}{}/", path_data, BGM_ROOT, pack);

    std::error_code error;
    if (!std::filesystem::is_directory(pack_path_, error)) {
      pack_path_.clear();
      return false;
    }
  } else {
    if (cur.empty()) {
      return true;
    }
    pack_path_.clear();
  }

  if ((loaded_num_ != 0) && (BGM_Playing() != BGM_PLAYING::NONE)) {
    Play(loaded_num_ - 1);
  }
  return true;
}
