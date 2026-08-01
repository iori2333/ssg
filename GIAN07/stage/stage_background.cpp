///
/// StageBackground - validated tile map scrolling and stage background modes
///
#include <algorithm>
#include <limits>
#include <utility>

#include "stage_background.h"

#include "effect/effect_manager.h"
#include "gameplay/playfield.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

namespace stage {

bool TileMapScroller::Load(std::span<const uint8_t> data) {
  auto map = StageMap::Parse(data);
  if (!map) {
    return false;
  }

  const auto &tail = map->Layers().back();
  const uint64_t end =
      16ULL * (tail.rows.size() - kMapTailRows) * tail.scroll_wait;
  if (end > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    return false;
  }

  map_ = std::move(*map);
  positions_.assign(map_->Layers().size(), {});
  speed_ = kDefaultScrollSpeed;
  count_ = 0;
  end_ = static_cast<int64_t>(end);
  return true;
}

void TileMapScroller::Update() {
  if (!map_ || count_ >= end_ || speed_ == 0) {
    return;
  }

  count_ += speed_;
  const auto &layers = map_->Layers();
  for (size_t i = 0; i < layers.size(); ++i) {
    auto &position = positions_[i];
    const auto wait = static_cast<int64_t>(layers[i].scroll_wait);
    position.count += speed_;
    if (speed_ > 0) {
      while (position.count >= wait) {
        position.count -= wait;
        position.dy = static_cast<uint8_t>((position.dy + 1) % 16);
        if (position.dy == 0 &&
            position.row + kVisibleMapRows < layers[i].rows.size()) {
          ++position.row;
        }
      }
    } else {
      while (position.count < 0) {
        if (position.dy == 0 && position.row > 0) {
          --position.row;
        }
        position.count += wait;
        position.dy = static_cast<uint8_t>((position.dy + 15) % 16);
      }
    }
  }
}

void TileMapScroller::Draw(const std::array<int8_t, kVisibleMapRows> &raster_dx,
                           int quake_dx) const {
  if (!map_) {
    return;
  }

  const auto &layers = map_->Layers();
  for (size_t layer_index = 0; layer_index < layers.size(); ++layer_index) {
    const auto &layer = layers[layer_index];
    const auto &position = positions_[layer_index];
    for (size_t screen_row = 0; screen_row < kVisibleMapRows; ++screen_row) {
      const size_t map_row = position.row + screen_row;
      if (map_row >= layer.rows.size()) {
        break;
      }
      const int row_y = 29 - static_cast<int>(screen_row);
      const int raster =
          layer_index == 0 ? raster_dx[kVisibleMapRows - 1 - screen_row] : 0;
      for (size_t column = 0; column < kMapWidth; ++column) {
        const uint16_t tile = layer.rows[map_row][column];
        if (tile == kEmptyMapTile) {
          continue;
        }
        const int x = (static_cast<int>(column) << 4) + playfield::kLeft +
                      quake_dx + raster;
        const int y = (row_y << 4) + position.dy;
        const int source_x = (tile % (640 / 16)) << 4;
        const int source_y = (tile / (640 / 16)) << 4;
        const PixelLtrb source = {source_x, source_y, source_x + 16,
                                  source_y + 16};
        GraphicsSurfaceBlit({x, y}, SurfaceId::MapChip, source);
      }
    }
  }
}

bool StageBackground::LoadMap(std::span<const uint8_t> data) {
  TileMapScroller map;
  if (!map.Load(data)) {
    return false;
  }
  map_ = std::move(map);
  ResetEffects();
  return true;
}

void StageBackground::ResetEffects() {
  mode_ = Mode::TileMap;
  raster_dx_.fill(0);
  effect_count_ = 0;
  quake_ = 0;
  raster_width_ = 0;
  raster_angle_ = 0;
}

void StageBackground::Update(EffectManager &effects) {
  switch (mode_) {
  case Mode::TileMap:
    break;
  case Mode::Stage2Boss:
    UpdateStage2Boss();
    break;
  case Mode::RasterOpening:
    UpdateRaster(true);
    break;
  case Mode::RasterClosing:
    UpdateRaster(false);
    break;
  case Mode::Stage3Boss:
    effect_count_ = (effect_count_ + 200) % 208;
    break;
  case Mode::Stage6Cube:
    visuals_.UpdateCubes();
    break;
  case Mode::Stage6RandomEcl:
    visuals_.UpdateFakeEcl();
    break;
  case Mode::Stage4Rock:
    visuals_.UpdateRocks();
    break;
  case Mode::Stage6Raster:
    visuals_.UpdateRasters();
    break;
  case Mode::Stage3Stars:
    ++effect_count_;
    if (effect_count_ == 32) {
      effects.StartScreenTransition(ScreenTransition::WhiteOut);
    }
    visuals_.UpdateStars();
    break;
  }

  if (quake_ != 0) {
    quake_ = static_cast<uint8_t>(quake_ + 2);
  }
  map_.Update();
}

void StageBackground::Draw() const {
  switch (mode_) {
  case Mode::Stage3Boss:
    DrawStage3Boss();
    return;
  case Mode::Stage6Cube:
    visuals_.DrawCubes();
    return;
  case Mode::Stage6RandomEcl:
    visuals_.DrawFakeEcl();
    return;
  case Mode::Stage6Raster:
    visuals_.DrawRasters();
    return;
  case Mode::Stage3Stars:
    visuals_.DrawStars();
    return;
  default:
    break;
  }

  const int quake_dx =
      quake_ == 0
          ? 0
          : math::RoundedPolarVector(static_cast<float>(quake_ * 16) *
                                         math::kLegacyAngleStep,
                                     static_cast<float>((256 - quake_) >> 5))
                .y;
  map_.Draw(raster_dx_, quake_dx);
  if (mode_ == Mode::Stage4Rock) {
    visuals_.DrawRocks();
  }
}

void StageBackground::Command(BackgroundCommand command,
                              EffectManager &effects) {
  switch (command) {
  case BackgroundCommand::Quake:
    quake_ = 2;
    break;
  case BackgroundCommand::Stage2Boss:
    mode_ = Mode::Stage2Boss;
    effect_count_ = 0;
    break;
  case BackgroundCommand::RasterOn:
    mode_ = Mode::RasterOpening;
    raster_angle_ = 0;
    raster_width_ = 0;
    break;
  case BackgroundCommand::RasterOff:
    mode_ = Mode::RasterClosing;
    break;
  case BackgroundCommand::Stage3Boss:
    mode_ = Mode::Stage3Boss;
    effect_count_ = 0;
    break;
  case BackgroundCommand::Stage3Reset:
    mode_ = Mode::TileMap;
    effect_count_ = 0;
    break;
  case BackgroundCommand::Stage6Cube:
    mode_ = Mode::Stage6Cube;
    effect_count_ = 0;
    visuals_.StartCubes();
    break;
  case BackgroundCommand::Stage6RandomEcl:
    mode_ = Mode::Stage6RandomEcl;
    effect_count_ = 0;
    visuals_.StartFakeEcl();
    break;
  case BackgroundCommand::Stage4Rock:
    mode_ = Mode::Stage4Rock;
    effect_count_ = 0;
    visuals_.StartRocks();
    break;
  case BackgroundCommand::Stage4Leave:
    if (mode_ == Mode::Stage4Rock) {
      visuals_.CommandRocks(Stage4RockCommand::Leave);
    }
    break;
  case BackgroundCommand::Stage6Raster:
    mode_ = Mode::Stage6Raster;
    effect_count_ = 0;
    visuals_.StartRasters();
    break;
  case BackgroundCommand::Stage3Stars:
    mode_ = Mode::Stage3Stars;
    effect_count_ = 0;
    visuals_.StartStars();
    effects.StartScreenTransition(ScreenTransition::WhiteIn);
    break;
  }
}

void StageBackground::CommandRocks(Stage4RockCommand command) {
  if (mode_ == Mode::Stage4Rock) {
    visuals_.CommandRocks(command);
  }
}

void StageBackground::UpdateStage2Boss() {
  struct SpeedChange {
    uint32_t frame;
    int speed;
  };
  static constexpr std::array kSpeedChanges = {
      SpeedChange{0, 1512},    SpeedChange{20, 1200},   SpeedChange{40, 900},
      SpeedChange{60, 600},    SpeedChange{80, 300},    SpeedChange{100, 150},
      SpeedChange{140, -150},  SpeedChange{160, -300},  SpeedChange{180, -600},
      SpeedChange{200, -900},  SpeedChange{220, -1200}, SpeedChange{240, -1512},
      SpeedChange{440, -1512}, SpeedChange{460, -1200}, SpeedChange{480, -900},
      SpeedChange{500, -600},  SpeedChange{520, -300},  SpeedChange{540, -150},
      SpeedChange{580, 150},   SpeedChange{600, 300},   SpeedChange{620, 600},
      SpeedChange{640, 900},   SpeedChange{660, 1200},  SpeedChange{680, 1512},
  };
  const auto found =
      std::ranges::find(kSpeedChanges, effect_count_, &SpeedChange::frame);
  if (found != kSpeedChanges.end()) {
    map_.SetSpeed(found->speed);
  }
  effect_count_ = (effect_count_ + 1) % 880;
}

void StageBackground::UpdateRaster(bool opening) {
  const int angle_step = opening ? 16 : 2;
  for (size_t i = 0; i < raster_dx_.size(); ++i) {
    const auto offset = math::RoundedPolarVector(
        static_cast<float>(raster_angle_ + static_cast<int>(i) * angle_step) *
            math::kLegacyAngleStep,
        static_cast<float>(raster_width_));
    raster_dx_[i] = static_cast<int8_t>(offset.y);
  }
  raster_angle_ = static_cast<uint8_t>(raster_angle_ + (opening ? 2 : 8));
  if (opening) {
    if (raster_width_ < 2) {
      ++raster_width_;
    }
  } else if (raster_width_ > 0) {
    --raster_width_;
    if (raster_width_ == 0) {
      mode_ = Mode::TileMap;
    }
  } else {
    mode_ = Mode::TileMap;
  }
}

void StageBackground::DrawStage3Boss() const {
  for (int y = playfield::kTop - static_cast<int>(effect_count_);
       y < playfield::kBottom; y += 208) {
    constexpr PixelLtrb source = {0, 272, (640 - 256), (272 + 208)};
    GraphicsSurfaceBlit({playfield::kLeft, y}, SurfaceId::MapChip, source);
  }
}

} // namespace stage
