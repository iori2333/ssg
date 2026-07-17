///
/// GfxManager — stage/face/enemy image loading
///
#pragma once

#include <cstdint>

#include "core/constants.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"

struct FaceData { PALETTE pal; };
struct EndingGfx { PIXEL_LTRB rcTarget; PALETTE pal; };

inline constexpr auto kGfxMusicRoom  = 128;
inline constexpr auto kGfxTitle      = 129;
inline constexpr auto kGfxNameRegist = 130;
inline constexpr auto kGfxExStage    = 131;
inline constexpr auto kGfxExBoss1    = 132;
inline constexpr auto kGfxExBoss2    = 133;
inline constexpr auto kGfxSProject   = 134;
inline constexpr auto kGfxEnding     = 135;
inline constexpr auto kFaceNumX      = 6;

class GfxManager {
public:
  bool LoadStage(int stage);
  void ReloadStage();
  bool LoadEnemySurface(uint8_t image_no);
  bool LoadGalleryEnemySurfaces();
  bool LoadFace(uint8_t face_id, uint8_t file_no);

  FaceData face_palettes[FACE_MAX]{};
  EndingGfx ending_gfx[ENDING_PIC_MAX]{};
  PALETTE sp_project_palette{};

private:
  int loaded_stage_ = 0;
  PALETTE enemy_palette_{};
};

inline GfxManager gfx;
