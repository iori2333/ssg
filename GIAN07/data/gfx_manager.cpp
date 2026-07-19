///
/// GfxManager — stage/face/enemy image loading
///
#include "gfx_manager.h"

#include <cassert>

#include "core/config.h"
#include "gfx/format_bmp.h"
#include "gfx/graphics_backend.h"
#include "pack_manager.h"

namespace {

bool GrpLoadBmp(const PackFile &in, uint32_t filno, SURFACE_ID sid) {
  auto maybe_bmp = BMPLoad(in.Extract(filno));
  assert(maybe_bmp);
  if (!maybe_bmp) {
    return false;
  }
  return GrpSurface_Load(sid, std::move(maybe_bmp.value()));
}

} // namespace

bool GfxManager::LoadStage(GameStage stage) {
  loaded_stage_ = stage;
  const auto &graph = packs.Images();

  if (stage == GameStage::MUSIC_ROOM) {
    return GrpLoadBmp(graph, 0, SURFACE_ID::SYSTEM) &&
           GrpLoadBmp(graph, 19 + 4, SURFACE_ID::MUSIC);
  }
  if (stage == GameStage::TITLE) {
    return GrpLoadBmp(graph, 0, SURFACE_ID::SYSTEM) &&
           GrpLoadBmp(graph, 20 + 4, SURFACE_ID::TITLE);
  }
  if (stage == GameStage::NAME_REGIST) {
    return GrpLoadBmp(graph, 0, SURFACE_ID::SYSTEM) &&
           GrpLoadBmp(graph, 21 + 4, SURFACE_ID::NAMEREG);
  }
  if (stage == GameStage::S_PROJECT) {
    if (!GrpLoadBmp(graph, 31, SURFACE_ID::SPROJECT)) {
      return false;
    }
    GrpBackend_PaletteGet(sp_project_palette);
    return true;
  }
  if (stage == GameStage::ENDING) {
    if (!GrpLoadBmp(graph, 32, SURFACE_ID::ENDING_CREDITS)) {
      return false;
    }
    for (auto i = 0; i < ENDING_PIC_MAX; i++) {
      if (!GrpLoadBmp(graph, 33 + i, SURFACE_ID::ENDING_PIC + i)) {
        return false;
      }
      GrpBackend_PaletteGet(ending_gfx[i].pal);
    }
    return true;
  }
  if (stage == GameStage::EXTRA) {
    if (!GrpLoadBmp(graph, 0, SURFACE_ID::SYSTEM) ||
        !GrpLoadBmp(graph, 27 + 1, SURFACE_ID::ENEMY) ||
        !GrpLoadBmp(graph, 27, SURFACE_ID::MAPCHIP) ||
        !GrpLoadBmp(graph, 26, SURFACE_ID::BOMBER)) {
      return false;
    }
    GrpBackend_PaletteGet(enemy_palette_);
    return true;
  }
  if (stage == GameStage::EX_BOSS1) {
    if (!GrpLoadBmp(graph, 29, SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(enemy_palette_);
    return true;
  }
  if (stage == GameStage::EX_BOSS2) {
    if (!GrpLoadBmp(graph, 30, SURFACE_ID::ENEMY)) {
      return false;
    }
    GrpBackend_PaletteGet(enemy_palette_);
    return true;
  }

  const auto stage_val = std::to_underlying(stage);
  if (stage_val < 1 || stage_val > STAGE_MAX) {
    return false;
  }

  const uint32_t kMapChipId[STAGE_MAX] = {7, 8, 9, 10, 11, 12};
  if (!GrpLoadBmp(graph, 0, SURFACE_ID::SYSTEM) ||
      !GrpLoadBmp(graph, stage_val + 0, SURFACE_ID::ENEMY) ||
      !GrpLoadBmp(graph, kMapChipId[stage_val - 1], SURFACE_ID::MAPCHIP) ||
      !GrpLoadBmp(graph, 26, SURFACE_ID::BOMBER)) {
    return false;
  }
  GrpBackend_PaletteGet(enemy_palette_);
  return true;
}

void GfxManager::ReloadStage() {
  assert(loaded_stage_ != GameStage::NONE);
  LoadStage(loaded_stage_);
}

bool GfxManager::LoadEnemySurface(uint8_t image_no) {
  return GrpLoadBmp(packs.Images(), image_no, SURFACE_ID::ENEMY);
}

bool GfxManager::LoadGalleryEnemySurfaces() {
  const auto &graph = packs.Images();
  auto bmp29 = BMPLoad(graph.Extract(29));
  auto bmp30 = BMPLoad(graph.Extract(30));
  if (!bmp29 || !bmp30) {
    return false;
  }

  auto &b29 = bmp29.value();
  auto &b30 = bmp30.value();
  const int src_stride = static_cast<int>(b30.info.Stride());
  const int dst_stride = static_cast<int>(b29.info.Stride());
  const auto src_w = b30.info.biWidth;
  const auto src_h = b30.info.biHeight;
  const auto dst_w = b29.info.biWidth;
  const auto dst_h = b29.info.biHeight;
  const int copy_y = 320;
  const int copy_h = 64;
  const int copy_w = std::min<int>(src_w, dst_w);

  for (int y = 0; y < copy_h; y++) {
    const int src_y = src_h - 1 - (copy_y + y);
    const int dst_y = dst_h - 1 - (copy_y + y);
    if (src_y < 0 || src_y >= src_h || dst_y < 0 || dst_y >= dst_h) {
      continue;
    }
    for (int x = 0; x < copy_w; x++) {
      const auto pixel = b30.pixels[src_y * src_stride + x];
      if (pixel != std::byte{0}) {
        b29.pixels[dst_y * dst_stride + x] = pixel;
      }
    }
  }
  return GrpSurface_Load(SURFACE_ID::ENEMY, std::move(b29));
}

bool GfxManager::LoadFace(uint8_t face_id, uint8_t file_no) {
  if (face_id >= FACE_MAX) {
    return false;
  }
  if (!GrpLoadBmp(packs.Images(), 13 + file_no, SURFACE_ID::FACE + face_id)) {
    return false;
  }
  GrpBackend_PaletteGet(face_palettes[face_id].pal);
  return true;
}
