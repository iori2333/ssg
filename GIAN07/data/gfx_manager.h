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

enum class AssetId : uint8_t {
  STAGE_1 = 0,
  STAGE_2,
  STAGE_3,
  STAGE_4,
  STAGE_5,
  STAGE_6,
  EXTRA,
  ENDING,
  MUSIC_ROOM,
  TITLE,
  NAME_REGIST,
  S_PROJECT,
};

constexpr AssetId StageToAssetId(StageId s) {
  return static_cast<AssetId>(std::to_underlying(s));
}

class GfxManager {
public:
  bool LoadStage(AssetId stage);
  void ReloadStage();
  bool LoadEnemySurface(uint8_t image_no);
  bool LoadGalleryEnemySurfaces();
  bool LoadSystemSurface();
  void SwapEnemySurface(uint8_t image_no);
  bool LoadFace(uint8_t face_id, uint8_t file_no);

  FaceData face_palettes[FACE_MAX]{};
  EndingGfx ending_gfx[ENDING_PIC_MAX]{};
  PALETTE sp_project_palette{};

private:
  AssetId loaded_stage_ = AssetId::STAGE_1;
  PALETTE enemy_palette_{};
};

inline GfxManager gfx;
