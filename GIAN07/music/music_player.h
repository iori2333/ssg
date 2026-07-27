///
/// MusicPlayer - track playback, metadata, and BGM pack selection
///
/// Orchestrates the loading pipeline: pick a track → load MIDI (always,
/// for sequencer + fallback audio) → optionally try waveform from a BGM
/// pack → play.
///
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "data/game_data.h"

class MusicPlayer {
public:
  explicit MusicPlayer(const data::GameData &data) : data_(data) {}

  // Track switching. Loads the MIDI track from MUSIC.PAK, then tries to
  // open a replacement waveform from the active BGM pack. Plays on success.
  bool Play(unsigned int id);

  // Track title: waveform metadata title first, then GameData fallback.
  std::string_view CurrentTitle() const;

  // Total number of tracks in MUSIC.PAK.
  size_t TrackCount() const;

  // BGM pack management
  bool HasPacks(bool invalidate_cache = false);
  void ForEachPack(std::function<void(std::string_view)> func);
  bool SetPack(std::string_view pack);

private:
  const data::GameData &data_;
  unsigned int loaded_num_ = 0; // 0 = nothing loaded
  std::string pack_path_;
  std::optional<bool> packs_available_{};
};
