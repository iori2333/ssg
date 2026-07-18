///
/// TrackManager — track selection, title lookup, BGM pack management
///
#include "track_manager.h"

#include <format>

#include <SDL3/SDL_filesystem.h>

#include "audio/bgm.h"
#include "data/music_manager.h"
#include "sys/file.h"
#include "sys/path.h"

static constexpr std::string_view BGM_ROOT = "bgm/";

// ---------------------------------------------------------------------------
// Track switching
// ---------------------------------------------------------------------------

bool TrackManager::Switch(unsigned int id) {
  if (!BGM_Enabled()) {
    return false;
  }
  BGM_Stop();
  BGM_ClearWaveform();

  // Always load MIDI for sequencer notes and fallback audio.
  if (!music.LoadTrack(id)) {
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

std::string_view TrackManager::CurrentTitle() const {
  auto wt = BGM_WaveformTitle();
  if (!wt.empty()) {
    return wt;
  }
  if (loaded_num_ > 0) {
    return music.Title(loaded_num_ - 1);
  }
  return {};
}

size_t TrackManager::TrackCount() const { return MusicManager::kTrackCount; }

// ---------------------------------------------------------------------------
// BGM pack management
// ---------------------------------------------------------------------------

static bool PackIterator(std::invocable<std::string_view> auto callback) {
  return SDL_EnumerateDirectory(
      BGM_ROOT.data(),
      [](void *cb, const char *bgm_root, const char *basename) {
        auto fn = std::format("{}{}", bgm_root, basename);
        if (!PathIsDirectory(fn.c_str())) {
          return SDL_ENUM_CONTINUE;
        }
        return (std::bit_cast<decltype(callback) *>(cb))
            ->operator()(std::string_view{basename});
      },
      &callback);
}

bool TrackManager::PacksAvailable(bool invalidate_cache) {
  if (packs_available_.has_value() && !invalidate_cache) {
    return packs_available_.value();
  }
  packs_available_ =
      PackIterator([](std::string_view) { return SDL_ENUM_SUCCESS; });
  return packs_available_.value();
}

size_t TrackManager::PackCount() {
  size_t ret = 0;
  PackIterator([&](std::string_view) {
    ret++;
    return SDL_ENUM_CONTINUE;
  });
  return ret;
}

void TrackManager::PackForeach(std::function<void(std::string_view)> func) {
  PackIterator([&](std::string_view pack) {
    func(pack);
    return SDL_ENUM_CONTINUE;
  });
}

bool TrackManager::PackSet(std::string_view pack) {
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

    if (!PathIsDirectory(pack_path_.c_str())) {
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
    Switch(loaded_num_ - 1);
  }
  return true;
}
