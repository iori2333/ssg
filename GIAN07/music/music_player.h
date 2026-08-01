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

namespace audio {
class AudioSystem;
}

enum class MidiVariant : uint8_t {
  Original,
  Arranged,
};

class MusicPlayer {
public:
  MusicPlayer(const data::GameData &data, audio::AudioSystem &audio)
      : data_(data), audio_(audio) {}

  // Track switching. Loads the MIDI track from game data, then tries to
  // open a replacement waveform from the active BGM pack. Plays on success.
  bool Play(unsigned int id);

  void SetMidiVariant(MidiVariant variant);

  // Total number of tracks in game data.
  [[nodiscard]] size_t TrackCount() const;

  // BGM pack management
  bool HasPacks(bool invalidate_cache = false);
  void ForEachPack(std::function<void(std::string_view)> func);
  bool SetPack(std::string_view pack);

private:
  const data::GameData &data_;
  audio::AudioSystem &audio_;
  unsigned int loaded_num_ = 0; // 0 = nothing loaded
  MidiVariant midi_variant_ = MidiVariant::Original;
  std::string pack_path_;
  std::optional<bool> packs_available_{};
};
