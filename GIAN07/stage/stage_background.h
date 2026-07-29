///
/// StageBackground - validated tile map scrolling and stage background modes
///
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "stage_map.h"
#include "stage_visuals.h"

class EffectManager;

namespace stage {

inline constexpr int kDefaultScrollSpeed = 20;

enum class BackgroundCommand : uint8_t {
  Quake = 0x01,
  Stage2Boss = 0x02,
  RasterOn = 0x03,
  RasterOff = 0x04,
  Stage3Boss = 0x05,
  Stage3Reset = 0x06,
  Stage6Cube = 0x07,
  Stage6RandomEcl = 0x08,
  Stage4Rock = 0x09,
  Stage4Leave = 0x0a,
  Stage6Raster = 0x0b,
  Stage3Stars = 0x0c,
};

class TileMapScroller {
public:
  [[nodiscard]] bool Load(std::span<const uint8_t> data);
  void Update();
  void Draw(const std::array<int8_t, kVisibleMapRows> &raster_dx,
            int quake_dx) const;
  void SetSpeed(int speed) { speed_ = speed; }
  [[nodiscard]] bool Loaded() const { return map_.has_value(); }

private:
  struct LayerPosition {
    size_t row = 0;
    int64_t count = 0;
    uint8_t dy = 0;
  };

  std::optional<StageMap> map_;
  std::vector<LayerPosition> positions_;
  int speed_ = kDefaultScrollSpeed;
  int64_t count_ = 0;
  int64_t end_ = 0;
};

class StageBackground {
public:
  [[nodiscard]] bool LoadMap(std::span<const uint8_t> data);
  void Update(EffectManager &effects);
  void Draw() const;
  void Command(BackgroundCommand command, EffectManager &effects);
  void CommandRocks(Stage4RockCommand command);
  void SetSpeed(int speed) { map_.SetSpeed(speed); }

private:
  enum class Mode : uint8_t {
    TileMap,
    Stage2Boss,
    RasterOpening,
    RasterClosing,
    Stage3Boss,
    Stage6Cube,
    Stage6RandomEcl,
    Stage4Rock,
    Stage6Raster,
    Stage3Stars,
  };

  void ResetEffects();
  void UpdateStage2Boss();
  void UpdateRaster(bool opening);
  void DrawStage3Boss() const;

  TileMapScroller map_;
  Mode mode_ = Mode::TileMap;
  std::array<int8_t, kVisibleMapRows> raster_dx_{};
  uint32_t effect_count_ = 0;
  uint8_t quake_ = 0;
  uint8_t raster_width_ = 0;
  uint8_t raster_angle_ = 0;
  StageVisuals visuals_;
};

} // namespace stage
