///
/// TrackManager — track selection, title lookup, BGM pack management
///
/// Orchestrates the loading pipeline: pick a track → load MIDI (always,
/// for sequencer + fallback audio) → optionally try waveform from a BGM
/// pack → play.
///
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <functional>
#include <string_view>

class TrackManager {
public:
  // Track switching. Loads the MIDI track from MUSIC.PAK, then tries to
  // open a replacement waveform from the active BGM pack. Plays on success.
  bool Switch(unsigned int id);

  // Track title: waveform metadata title first, then MusicManager fallback.
  std::string_view CurrentTitle() const;

  // Total number of tracks in MUSIC.PAK.
  size_t TrackCount() const;

  // BGM pack management
  bool PacksAvailable(bool invalidate_cache = false);
  size_t PackCount();
  void PackForeach(std::function<void(std::string_view)> func);
  bool PackSet(std::string_view pack);

private:
  unsigned int loaded_num_ = 0;  // 0 = nothing loaded
  std::string pack_path_;
  std::optional<bool> packs_available_{};
};

inline TrackManager track_mgr;
