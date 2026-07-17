///
/// PackManager — owns and loads all .PAK game archives
///
#include "pack_manager.h"

#include <SDL3/SDL_iostream.h>

#include "data/music_manager.h"
#include "data/sfx_manager.h"
#include "sys/path.h"

bool PackArchive::Load(std::string_view data_path) {
  if (data_) {
    return true;
  }
  const auto full = std::string(data_path) + filename_;
  auto *stream = SDL_IOFromFile(full.c_str(), "rb");
  if (stream == nullptr) {
    return false;
  }
  auto in = FilStartR(stream);
  if (on_load_) {
    if (!on_load_(in)) {
      return false;
    }
  }
  data_ = std::move(in);
  return true;
}

bool PackManager::LoadAll() {
  // Lazy-init the packs array on first call (resolves cross-manager deps)
  static bool initialized = false;
  if (!initialized) {
    packs_ = {{
        PackArchive("MAP.PAK"),
        PackArchive("IMAGES.PAK"),
        PackArchive("MUSIC.PAK", MusicManager::LoadMetadata),
        PackArchive("SOUND.PAK", [](const PACKFILE_READ &in) {
          return SfxManager::LoadAllFromPack(in);
        }),
    }};
    initialized = true;
  }

  bool ret = true;
  for (auto &p : packs_) {
    ret &= p.Load(PathForData());
  }
  return ret;
}

std::string PackManager::MissingFilesReport() const {
  std::string result;
  for (const auto &p : packs_) {
    result += p.Path();
    result += '\n';
  }
  return result;
}
