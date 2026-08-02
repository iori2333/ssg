///
/// GraphicsLoader - installs game image assets into graphics surfaces
///
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "graphics_loader.h"

#include "gameplay/game_rules.h"
#include "gfx/constants.h"
#include "gfx/format_bmp.h"
#include "gfx/graphics_backend.h"
#include "graphics_assets.h"

namespace data {

bool GraphicsLoader::LoadBmp(uint32_t file_no, SurfaceId surface) const {
  auto bmp = BmpLoad(data_->ExtractImage(file_no));
  return bmp && GraphicsSurfaceLoad(surface, std::move(*bmp));
}

bool GraphicsLoader::Load(Set set) {
  bool loaded = false;
  switch (set) {
  case Set::MusicRoom:
    loaded = LoadBmp(0, SurfaceId::System) && LoadBmp(23, SurfaceId::Music);
    break;
  case Set::Title:
    loaded = LoadBmp(0, SurfaceId::System) && LoadBmp(24, SurfaceId::Title);
    break;
  case Set::NameRegistration:
    loaded = LoadBmp(0, SurfaceId::System) &&
             LoadBmp(25, SurfaceId::NameRegistration);
    break;
  case Set::ProjectScreen:
    loaded = LoadBmp(31, SurfaceId::Project);
    break;
  case Set::Ending:
    loaded = LoadBmp(32, SurfaceId::EndingCredits);
    for (uint8_t i = 0; loaded && i < graphics_assets::kEndingPictureCount;
         ++i) {
      loaded = LoadBmp(33 + i, graphics_assets::EndingPictureSurface(i));
    }
    break;
  case Set::Extra:
    loaded = LoadBmp(0, SurfaceId::System) && LoadBmp(28, SurfaceId::Enemy) &&
             LoadBmp(27, SurfaceId::MapChip) && LoadBmp(26, SurfaceId::Bomber);
    break;
  case Set::BulletGallery:
    loaded = LoadBmp(0, SurfaceId::System) && LoadGalleryEnemySurface();
    break;
  default: {
    const auto stage = std::to_underlying(set);
    constexpr std::array<uint32_t, 6> kMapChipIds = {7, 8, 9, 10, 11, 12};
    loaded = LoadBmp(0, SurfaceId::System) &&
             LoadBmp(stage + 1, SurfaceId::Enemy) &&
             LoadBmp(kMapChipIds[stage], SurfaceId::MapChip) &&
             LoadBmp(26, SurfaceId::Bomber);
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
  if (value > std::to_underlying(StageId::Extra)) {
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
  return LoadBmp(image_no, SurfaceId::Enemy);
}

bool GraphicsLoader::LoadGalleryEnemySurface() const {
  auto bmp29 = BmpLoad(data_->ExtractImage(29));
  auto bmp30 = BmpLoad(data_->ExtractImage(30));
  if (!bmp29 || !bmp30) {
    return false;
  }

  const int src_stride = static_cast<int>(bmp30->info.Stride());
  const int dst_stride = static_cast<int>(bmp29->info.Stride());
  const int copy_y = 320;
  const int copy_h = 64;
  const int copy_w = std::min<int>(bmp30->info.biWidth, bmp29->info.biWidth);
  const auto src_pixels = bmp30->Pixels();
  auto dst_pixels = bmp29->Pixels();

  for (int y = 0; y < copy_h; ++y) {
    const int src_y = bmp30->info.biHeight - 1 - (copy_y + y);
    const int dst_y = bmp29->info.biHeight - 1 - (copy_y + y);
    if (src_y < 0 || src_y >= bmp30->info.biHeight || dst_y < 0 ||
        dst_y >= bmp29->info.biHeight) {
      continue;
    }
    for (int x = 0; x < copy_w; ++x) {
      const auto pixel = src_pixels[src_y * src_stride + x];
      if (pixel != std::byte{0}) {
        dst_pixels[dst_y * dst_stride + x] = pixel;
      }
    }
  }
  return GraphicsSurfaceLoad(SurfaceId::Enemy, std::move(*bmp29));
}

bool GraphicsLoader::LoadFace(uint8_t face_id, uint8_t file_no) {
  return face_id < graphics_assets::kFaceSurfaceCount &&
         LoadBmp(13 + file_no, graphics_assets::FaceSurface(face_id));
}

} // namespace data
