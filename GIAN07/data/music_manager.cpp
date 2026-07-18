///
/// MusicManager — music metadata + MIDI loading
///
#include "music_manager.h"

#include <cassert>
#include <cstring>

#include "audio/midi.h"
#include "pack_manager.h"
#include "util/endian.h"

bool MusicManager::LoadMetadata(const PackFile &in) {
  music.metas_.resize(kTrackCount);

  for (auto i = 0; std::cmp_less(i, kTrackCount); i++) {
    if (const auto file = in.Extract(i)) {
      auto cursor = file.cursor();

      std::string_view title, comment;
      if (const auto title_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
        const auto title_len = title_len_val.value()[0];
        if (cursor.cursor + title_len <= cursor.size()) {
          title = {reinterpret_cast<const char *>(&cursor[cursor.cursor]),
                   title_len};
          cursor.next<uint8_t>(title_len);
        }
      }
      if (const auto comment_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
        const auto comment_len = comment_len_val.value()[0];
        if (cursor.cursor + comment_len <= cursor.size()) {
          comment = {reinterpret_cast<const char *>(&cursor[cursor.cursor]),
                     comment_len};
          cursor.next<uint8_t>(comment_len);
        }
      }

      music.metas_[i].title = title;
      music.metas_[i].comment = comment;
    } else {
      assert(!"Failure extracting BGM file?");
    }
  }
  return true;
}

bool MusicManager::LoadTrack(unsigned int index) {
  const auto &music_pack = packs.Music();
  if (index >= metas_.size()) {
    return false;
  }

  auto raw = music_pack.Extract(index);
  if (!raw) {
    return false;
  }

  auto cursor = raw.cursor();
  if (const auto title_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
    cursor.next<uint8_t>(title_len_val.value()[0]);
  }
  if (const auto comment_len_val = cursor.next<ENDIAN_LITTLE<uint32_t>>()) {
    cursor.next<uint8_t>(comment_len_val.value()[0]);
  }

  const auto midi_size = raw.size() - cursor.cursor;
  BYTE_BUFFER_OWNED midi_buf(midi_size);
  if (midi_buf) {
    std::memcpy(midi_buf.get(), raw.get() + cursor.cursor, midi_size);
  }

  return Mid_Load(std::move(midi_buf));
}

bool MusicManager::LoadTrackByIndex(int index) {
  if ((index < 0) || (index >= static_cast<int>(metas_.size()))) {
    return false;
  }
  return LoadTrack(index);
}

std::string_view MusicManager::Title(unsigned int index) const {
  if (index < metas_.size()) {
    return metas_[index].title;
  }
  return {};
}

std::string_view MusicManager::Comment(unsigned int index) const {
  if (index < metas_.size()) {
    return metas_[index].comment;
  }
  return {};
}

BYTE_BUFFER_OWNED MusicManager::LoadRoomComment(int index) const {
  if ((index < 0) || (index >= static_cast<int>(metas_.size()))) {
    return nullptr;
  }
  const auto &comment = metas_[index].comment;
  BYTE_BUFFER_OWNED buf(comment.size());
  if (buf) {
    std::memcpy(buf.get(), comment.data(), comment.size());
  }
  return buf;
}
