///
/// GraphicsLoader - installs game image assets into graphics surfaces
///
#pragma once

#include <cstdint>
#include <optional>

#include "game_data.h"

#include "gameplay/game_rules.h"
#include "gfx/constants.h"

namespace data {

class GraphicsLoader {
public:
  explicit GraphicsLoader(const GameData &data) : data_(&data) {}

  [[nodiscard]] bool LoadStage(StageId stage);
  [[nodiscard]] bool LoadTitle();
  [[nodiscard]] bool LoadNameRegistration();
  [[nodiscard]] bool LoadMusicRoom();
  [[nodiscard]] bool LoadProjectScreen();
  [[nodiscard]] bool LoadEnding();
  [[nodiscard]] bool LoadBulletGallery();
  [[nodiscard]] bool LoadFace(uint8_t face_id, uint8_t file_no);
  [[nodiscard]] bool SwapEnemySurface(uint8_t image_no);
  [[nodiscard]] bool Reload();

private:
  enum class Set : uint8_t {
    Stage1,
    Stage2,
    Stage3,
    Stage4,
    Stage5,
    Stage6,
    Extra,
    Ending,
    MusicRoom,
    Title,
    NameRegistration,
    ProjectScreen,
    BulletGallery,
  };

  [[nodiscard]] bool Load(Set set);
  [[nodiscard]] bool LoadBmp(uint32_t file_no, SurfaceId surface) const;
  [[nodiscard]] bool LoadGalleryEnemySurface() const;

  const GameData *data_;
  std::optional<Set> loaded_set_;
};

} // namespace data
