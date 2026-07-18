///
/// MusicManager — music metadata + MIDI loading
///
#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "core/lz_uty.h"
#include "sys/buffer.h"

class MusicManager {
public:
  static constexpr size_t kTrackCount = 20;

  bool LoadTrack(unsigned int index);
  bool LoadTrackByIndex(int index);
  std::string_view Title(unsigned int index) const;
  std::string_view Comment(unsigned int index) const;
  BYTE_BUFFER_OWNED LoadRoomComment(int index) const;

private:
  friend class PackManager;
  static bool LoadMetadata(const PackFile &in);

  struct Meta { std::string title, comment; };
  std::vector<Meta> metas_;
};

inline MusicManager music;
