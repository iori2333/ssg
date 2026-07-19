///
/// GfxManager — stage/face/enemy image loading
///
#pragma once

#include <cstdint>

#include "core/constants.h"
#include "core/game_manager.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"

struct FaceData { PALETTE pal; };
struct EndingGfx { PIXEL_LTRB rcTarget; PALETTE pal; };

inline constexpr auto kFaceNumX = 6;

class GfxManager {
public:
  bool LoadStage(GameStage stage);
  void ReloadStage();
  bool LoadEnemySurface(uint8_t image_no);
  bool LoadGalleryEnemySurfaces();
  bool LoadFace(uint8_t face_id, uint8_t file_no);

  FaceData face_palettes[FACE_MAX]{};
  EndingGfx ending_gfx[ENDING_PIC_MAX]{};
  PALETTE sp_project_palette{};

private:
  GameStage loaded_stage_ = GameStage::NONE;
  PALETTE enemy_palette_{};
};

inline GfxManager gfx;
