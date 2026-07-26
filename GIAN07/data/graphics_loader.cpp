///
/// GraphicsLoader - installs game image assets into graphics surfaces
///
#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

#include "graphics_loader.h"

#include "core/constants.h"
#include "gfx/format_bmp.h"
#include "gfx/graphics_backend.h"

namespace data {

bool GraphicsLoader::LoadBmp(uint32_t file_no, SURFACE_ID surface) const {
  auto bmp = BMPLoad(data_->ExtractImage(file_no));
  return bmp && GrpSurface_Load(surface, std::move(*bmp));
}

bool GraphicsLoader::Load(Set set) {
  bool loaded = false;
  switch (set) {
  case Set::MusicRoom:
    loaded = LoadBmp(0, SURFACE_ID::SYSTEM) && LoadBmp(23, SURFACE_ID::MUSIC);
    break;
  case Set::Title:
    loaded = LoadBmp(0, SURFACE_ID::SYSTEM) && LoadBmp(24, SURFACE_ID::TITLE);
    break;
  case Set::NameRegistration:
    loaded = LoadBmp(0, SURFACE_ID::SYSTEM) && LoadBmp(25, SURFACE_ID::NAMEREG);
    break;
  case Set::ProjectScreen:
    loaded = LoadBmp(31, SURFACE_ID::SPROJECT);
    break;
  case Set::Ending:
    loaded = LoadBmp(32, SURFACE_ID::ENDING_CREDITS);
    for (uint8_t i = 0; loaded && i < ENDING_PIC_MAX; ++i) {
      loaded = LoadBmp(33 + i, SURFACE_ID::ENDING_PIC + i);
    }
    break;
  case Set::Extra:
    loaded = LoadBmp(0, SURFACE_ID::SYSTEM) && LoadBmp(28, SURFACE_ID::ENEMY) &&
             LoadBmp(27, SURFACE_ID::MAPCHIP) &&
             LoadBmp(26, SURFACE_ID::BOMBER);
    break;
  case Set::BulletGallery:
    loaded = LoadBmp(0, SURFACE_ID::SYSTEM) && LoadGalleryEnemySurface();
    break;
  default: {
    const auto stage = std::to_underlying(set);
    constexpr std::array<uint32_t, 6> kMapChipIds = {7, 8, 9, 10, 11, 12};
    loaded = LoadBmp(0, SURFACE_ID::SYSTEM) &&
             LoadBmp(stage + 1, SURFACE_ID::ENEMY) &&
             LoadBmp(kMapChipIds[stage], SURFACE_ID::MAPCHIP) &&
             LoadBmp(26, SURFACE_ID::BOMBER);
    break;
  }
  }

  if (loaded) {
    loaded_set_ = set;
  }
  return loaded;
}

bool GraphicsLoader::LoadStage(StageId stage) {
  const auto value = std::to_underlying(stage);
  if (value > std::to_underlying(StageId::EXTRA)) {
    return false;
  }
  return Load(static_cast<Set>(value));
}

bool GraphicsLoader::LoadTitle() { return Load(Set::Title); }

bool GraphicsLoader::LoadNameRegistration() {
  return Load(Set::NameRegistration);
}

bool GraphicsLoader::LoadMusicRoom() { return Load(Set::MusicRoom); }

bool GraphicsLoader::LoadProjectScreen() { return Load(Set::ProjectScreen); }

bool GraphicsLoader::LoadEnding() { return Load(Set::Ending); }

bool GraphicsLoader::LoadBulletGallery() { return Load(Set::BulletGallery); }

bool GraphicsLoader::Reload() { return loaded_set_ && Load(*loaded_set_); }

bool GraphicsLoader::SwapEnemySurface(uint8_t image_no) {
  return LoadBmp(image_no, SURFACE_ID::ENEMY);
}

bool GraphicsLoader::LoadGalleryEnemySurface() const {
  auto bmp29 = BMPLoad(data_->ExtractImage(29));
  auto bmp30 = BMPLoad(data_->ExtractImage(30));
  if (!bmp29 || !bmp30) {
    return false;
  }

  const int src_stride = static_cast<int>(bmp30->info.Stride());
  const int dst_stride = static_cast<int>(bmp29->info.Stride());
  const int copy_y = 320;
  const int copy_h = 64;
  const int copy_w = std::min<int>(bmp30->info.biWidth, bmp29->info.biWidth);

  for (int y = 0; y < copy_h; ++y) {
    const int src_y = bmp30->info.biHeight - 1 - (copy_y + y);
    const int dst_y = bmp29->info.biHeight - 1 - (copy_y + y);
    if (src_y < 0 || src_y >= bmp30->info.biHeight || dst_y < 0 ||
        dst_y >= bmp29->info.biHeight) {
      continue;
    }
    for (int x = 0; x < copy_w; ++x) {
      const auto pixel = bmp30->pixels[src_y * src_stride + x];
      if (pixel != std::byte{0}) {
        bmp29->pixels[dst_y * dst_stride + x] = pixel;
      }
    }
  }
  return GrpSurface_Load(SURFACE_ID::ENEMY, std::move(*bmp29));
}

bool GraphicsLoader::LoadFace(uint8_t face_id, uint8_t file_no) {
  return face_id < FACE_MAX &&
         LoadBmp(13 + file_no, SURFACE_ID::FACE + face_id);
}

} // namespace data
