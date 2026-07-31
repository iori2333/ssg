///
/// Specialized animated stage backgrounds.
///

#include <array>
#include <cstddef>
#include <cstdint>

#include "stage_visuals.h"

#include "gameplay/playfield.h"
#include "gfx/coords.h"
#include "gfx/graphics_backend.h"
#include "util/math_utils.h"

namespace stage {

void StageVisuals::Transform(Point3D &point, uint8_t angle_x, uint8_t angle_y,
                             uint8_t angle_z) {
  auto rotated = math::RoundedRotateVector(math::AngleFromLegacy(angle_x),
                                           point.y, point.z);
  point.y = rotated.x;
  point.z = rotated.y;

  rotated = math::RoundedRotateVector(-math::AngleFromLegacy(angle_y), point.x,
                                      point.z);
  point.x = rotated.x;
  point.z = rotated.y;

  rotated = math::RoundedRotateVector(math::AngleFromLegacy(angle_z), point.x,
                                      point.y);
  point.x = rotated.x;
  point.y = rotated.y;
}

void StageVisuals::StartCubes() {
  constexpr int cube_count = static_cast<int>(kCubeCount);
  cube_phase_ = 0;
  cube_angle_x_ = 0;
  cube_angle_y_ = 0;
  cube_angle_z_ = 0;
  for (std::size_t index = 0; index < cubes_.size(); ++index) {
    auto &cube = cubes_[index];
    cube.half_size = 30_px;
    cube.rotation = {.x = math::RandomInt() & 0xff,
                     .y = math::RandomInt() & 0xff,
                     .z = math::RandomInt() & 0xff};
    const auto position =
        math::RoundedPolarVector(static_cast<float>(index) * math::kFullAngle /
                                     static_cast<float>(cube_count),
                                 200_px);
    cube.position.x = position.x;
    cube.position.y = position.y;
    cube.position.z = 0;
  }
  for (auto &star : cube_stars_) {
    star.x = math::RandomInt() % (640 - 256) + 128;
    star.y = -(math::RandomInt() % 480);
    star.velocity_y = math::RandomInt() % 10 + 10;
  }
}

void StageVisuals::UpdateCubes() {
  constexpr int cube_count = static_cast<int>(kCubeCount);
  cube_phase_ += 256;
  cube_angle_x_ += 128;
  cube_angle_y_ -= 64;

  const int angle_offset =
      math::RoundedPolarVector(static_cast<float>(cube_phase_ >> 8) *
                                   math::kLegacyAngleStep,
                               512.0f / static_cast<float>(cube_count))
          .y;
  const int radius =
      math::RoundedPolarVector(
          static_cast<float>(cube_phase_ >> 7) * math::kLegacyAngleStep, 100_px)
          .y +
      180_px;
  for (std::size_t index = 0; index < cubes_.size(); ++index) {
    auto &cube = cubes_[index];
    cube.half_size = 15_px + (radius >> 4) + static_cast<int>(index) * 128;
    cube.rotation.x += 4;
    cube.rotation.y -= 4;
    const auto position = math::RoundedPolarVector(
        static_cast<float>(static_cast<int>(index) * 500 / cube_count +
                           angle_offset) *
            math::kLegacyAngleStep,
        radius);
    cube.position.x = position.x;
    cube.position.y = position.y;
    cube.position.z =
        (static_cast<int>(index) - static_cast<int>(cubes_.size() / 2)) * 40_px;
    Transform(cube.position, cube_angle_x_ >> 8, cube_angle_y_ >> 8,
              cube_angle_z_ >> 8);
  }

  for (auto &star : cube_stars_) {
    star.y += star.velocity_y;
    if (star.y > 480) {
      star.x = math::RandomInt() % (640 - 256) + 128;
      star.y = 0;
      star.velocity_y = math::RandomInt() % 10 + 10;
    }
  }
}

void StageVisuals::DrawCubes() const {
  for (const auto &star : cube_stars_) {
    GrpSurface_Blit({star.x, star.y}, SURFACE_ID::SYSTEM,
                    PIXEL_LTWH{136, 272, 16, 24});
  }
  for (const auto &cube : cubes_) {
    DrawCube(cube);
  }
}

void StageVisuals::DrawCube(const Cube &cube) {
  const auto project = [&cube](Point3D point) {
    Transform(point, cube.rotation.x, cube.rotation.y, cube.rotation.z);
    point.x = ((point.x + cube.position.x) >> 6) + 320;
    point.y = ((point.y + cube.position.y) >> 6) + 240;
    return point;
  };
  const int length = cube.half_size;

  Geometry().SetColor({1, 1, 3});
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      const auto front = project({x * length, y * length, -length});
      const auto back = project({x * length, y * length, length});
      Geometry().DrawLine(front.x, front.y, back.x, back.y);
    }
  }
  Geometry().SetColor({0, 0, 3});
  for (int y = -1; y <= 1; ++y) {
    for (int z = -1; z <= 1; ++z) {
      const auto left = project({-length, y * length, z * length});
      const auto right = project({length, y * length, z * length});
      Geometry().DrawLine(left.x, left.y, right.x, right.y);
    }
  }
  Geometry().SetColor({1, 1, 4});
  for (int x = -1; x <= 1; ++x) {
    for (int z = -1; z <= 1; ++z) {
      const auto top = project({x * length, -length, z * length});
      const auto bottom = project({x * length, length, z * length});
      Geometry().DrawLine(top.x, top.y, bottom.x, bottom.y);
    }
  }
}

void StageVisuals::StartFakeEcl() {
  grid_offset_x_ = 320_px;
  grid_offset_y_ = 240_px;
  for (auto &line : fake_ecl_) {
    // Preserve the established RNG sequence used by later stage effects.
    (void)math::RandomInt();
    const int speed = math::RandomInt() % 5_px + 5_px;
    line.source_x = math::RandomInt() % 7 * 9 * 8;
    line.source_y = math::RandomInt() % 16 * 16;
    line.x = PixelToWorld(28 + math::RandomInt() % 484);
    line.y = -PixelToWorld(math::RandomInt() % 640);
    line.velocity_x = 0;
    line.velocity_y = speed;
  }
}

void StageVisuals::UpdateFakeEcl() {
  grid_offset_x_ = (grid_offset_x_ + 1) % 64;
  grid_offset_y_ = (grid_offset_y_ + 62) % 64;
  for (auto &line : fake_ecl_) {
    line.x += line.velocity_x;
    line.y += line.velocity_y;
    if (line.y < 480_px) {
      continue;
    }
    // Preserve the established RNG sequence used by later stage effects.
    (void)math::RandomInt();
    const int speed = math::RandomInt() % 5_px + 5_px;
    line.source_x = math::RandomInt() % 7 * 9 * 8;
    line.source_y = math::RandomInt() % 16 * 16;
    line.x = PixelToWorld(28 + math::RandomInt() % 484);
    line.y = -PixelToWorld(math::RandomInt() % 640);
    line.velocity_x = 0;
    line.velocity_y = speed;
  }
}

void StageVisuals::DrawFakeEcl() const {
  Geometry().SetColor({0, 2, 0});
  for (int x = 128 - grid_offset_x_ / 2; x < 512; x += 32) {
    Geometry().DrawLine(x, 0, x, 480);
  }
  for (int y = grid_offset_y_ / 2; y < 480; y += 32) {
    Geometry().DrawLine(128, y, 512, y);
  }
  Geometry().SetColor({0, 3, 0});
  for (int x = 128 - grid_offset_x_; x < 512; x += 64) {
    Geometry().DrawLine(x, 0, x, 480);
  }
  for (int y = -grid_offset_y_; y < 480; y += 64) {
    Geometry().DrawLine(128, y, 512, y);
  }

  for (const auto &line : fake_ecl_) {
    GrpSurface_Blit({line.x >> 6, line.y >> 6}, SURFACE_ID::MAPCHIP,
                    PIXEL_LTWH{line.source_x, line.source_y, 72, 16});
  }
  GrpSurface_Blit({128, 400}, SURFACE_ID::MAPCHIP,
                  PIXEL_LTRB{0, 272, 416, 352});
}

void StageVisuals::StartRocks() {
  constexpr int row_spacing = 500 * (64 / 4);
  int sprite = 2;
  for (std::size_t index = 0; index < rocks_.size(); ++index) {
    if (index == rocks_.size() * 5 / 8 || index == rocks_.size() * 7 / 8) {
      --sprite;
    }
    const int y_offset = static_cast<int>(index % 4) * row_spacing +
                         math::RandomInt() % (row_spacing / 2);
    const int x = math::RandomInt() % 500_px - 250_px;
    // Depth was never rendered, but this draw must remain in the RNG stream.
    (void)math::RandomInt();
    auto &rock = rocks_[index];
    rock = {.x = x,
            .y = -250_px - y_offset,
            .velocity_y = (4 - sprite) * 16,
            .speed = (4 - sprite) * 16,
            .sprite = static_cast<uint8_t>(sprite)};
  }
}

void StageVisuals::UpdateRocks() {
  const auto reset_above = [](Rock &rock, int velocity_scale) {
    rock.x = math::RandomInt() % 500_px - 250_px;
    rock.y = -PixelToWorld(290 + math::RandomInt() % 250);
    rock.velocity_y = (4 - rock.sprite) * velocity_scale;
    rock.speed = rock.velocity_y;
  };

  for (auto &rock : rocks_) {
    ++rock.age;
    switch (rock.state) {
    case RockState::Normal:
      rock.y += rock.velocity_y;
      if (rock.y > 290_px) {
        reset_above(rock, 16);
      }
      break;
    case RockState::Accelerating:
      rock.speed += rock.acceleration;
      rock.velocity_y = rock.speed;
      rock.y += rock.velocity_y;
      if (rock.y > 290_px) {
        reset_above(rock, 96);
        rock.acceleration = 0;
      }
      break;
    case RockState::Reversing:
      rock.speed -= rock.acceleration;
      rock.velocity_y = rock.speed;
      rock.y += rock.velocity_y;
      if (rock.age > 120) {
        if (rock.y > 290_px || rock.y < -290_px) {
          reset_above(rock, 32);
        }
        rock.velocity_y = (4 - rock.sprite) * 32;
        rock.speed = rock.velocity_y;
        rock.acceleration = 2;
        rock.state = RockState::Accelerating;
      } else if (rock.y > 290_px || rock.y < -290_px) {
        rock.x = math::RandomInt() % 500_px - 250_px;
        rock.y = 290_px + math::RandomInt() % 250;
        rock.velocity_y = -(4 - rock.sprite) * 96;
        rock.speed = rock.velocity_y;
        rock.acceleration = 0;
      }
      break;
    case RockState::Leaving:
      if (rock.y <= 540_px) {
        rock.y += rock.velocity_y;
      }
      break;
    case RockState::Ending:
      if (rock.y <= 540_px) {
        rock.y += (4 - rock.sprite) * 192;
      }
      break;
    }
  }
}

void StageVisuals::DrawRocks() const {
  static constexpr std::array sources = {PIXEL_LTRB{0, 224, 80, 288},
                                         PIXEL_LTRB{0, 288, 48, 336},
                                         PIXEL_LTRB{48, 288, 80, 320}};
  static constexpr std::array half_widths = {40, 24, 16};
  static constexpr std::array half_heights = {32, 24, 16};
  for (const auto &rock : rocks_) {
    const int x = (rock.x + playfield::kWorldCenterX) >> 6;
    const int y = (rock.y + playfield::kWorldCenterY) >> 6;
    GrpSurface_Blit(
        {x - half_widths[rock.sprite], y - half_heights[rock.sprite]},
        SURFACE_ID::MAPCHIP, sources[rock.sprite]);
  }
}

void StageVisuals::CommandRocks(Stage4RockCommand command) {
  for (auto &rock : rocks_) {
    switch (command) {
    case Stage4RockCommand::Accelerate:
      rock.state = RockState::Accelerating;
      rock.acceleration = static_cast<int8_t>(rock.speed / 24);
      rock.age = 0;
      break;
    case Stage4RockCommand::Reverse:
      rock.state = RockState::Reversing;
      rock.acceleration = static_cast<int8_t>(rock.speed / 12);
      rock.age = 0;
      break;
    case Stage4RockCommand::Leave:
      rock.state = RockState::Leaving;
      break;
    case Stage4RockCommand::End:
      rock.state = RockState::Ending;
      break;
    case Stage4RockCommand::Normal:
    case Stage4RockCommand::Rotate:
      break;
    }
  }
}

void StageVisuals::StartRasters() {
  for (std::size_t index = 0; index < rasters_.size(); ++index) {
    const int x = math::RandomInt() % (640 - 256) + 128;
    const int y = -(math::RandomInt() % (480 + 160));
    const auto angle = static_cast<uint8_t>(math::RandomInt());
    const auto amplitude = static_cast<uint8_t>(math::RandomInt() % 80 + 70);
    const auto velocity_y = static_cast<int8_t>(2 + math::RandomInt() % 3);
    rasters_[index] = {
        .x = x,
        .y = y,
        .velocity_y = velocity_y,
        .type = static_cast<uint8_t>(index % 3),
        .angle = angle,
        .amplitude = amplitude,
    };
  }
  for (auto &star : raster_stars_) {
    star = {.x = math::RandomInt() % (640 - 256) + 128,
            .y = math::RandomInt() % 480,
            .velocity_y = math::RandomInt() % 20 + 8};
  }
}

void StageVisuals::UpdateRasters() {
  for (std::size_t index = 0; index < rasters_.size(); ++index) {
    auto &raster = rasters_[index];
    raster.angle += static_cast<uint8_t>((index & 1) != 0 ? 2 : -2);
    raster.y += raster.velocity_y;
    if (raster.y > 480) {
      raster.x = math::RandomInt() % (640 - 256) + 128;
      raster.y = -160;
      raster.angle = static_cast<uint8_t>(math::RandomInt());
      raster.amplitude = static_cast<uint8_t>(math::RandomInt() % 80 + 70);
    }
  }
  for (auto &star : raster_stars_) {
    star.y += star.velocity_y;
    if (star.y > 480) {
      star.x = math::RandomInt() % (640 - 256) + 128;
      star.y -= 480;
      star.velocity_y = math::RandomInt() % 20 + 8;
    }
  }
}

void StageVisuals::DrawRasters() const {
  static constexpr std::array sources = {PIXEL_LTRB{608, 272, 640, 352},
                                         PIXEL_LTRB{592, 160, 640, 272},
                                         PIXEL_LTRB{576, 0, 640, 160}};
  for (const auto &star : raster_stars_) {
    GrpSurface_Blit({star.x, star.y}, SURFACE_ID::MAPCHIP,
                    PIXEL_LTRB{624, 352, 640, 368});
  }
  for (const auto &raster : rasters_) {
    const auto target = sources[raster.type];
    const int height = target.bottom - target.top;
    const int half_width = (target.right - target.left) / 2;
    for (int row = 0; row < height; row += 2) {
      const int offset =
          math::RoundedPolarVector(static_cast<float>(raster.angle + row) *
                                       math::kLegacyAngleStep,
                                   raster.amplitude)
              .y;
      GrpSurface_Blit(
          {raster.x + offset - half_width, raster.y + row}, SURFACE_ID::MAPCHIP,
          PIXEL_LTRB{target.left, target.top + row, target.right, row + 2});
    }
  }
}

void StageVisuals::StartStars() {
  for (auto &star : fast_stars_) {
    star = {.x = math::RandomInt() % (640 - 256) + 128,
            .y = math::RandomInt() % 480,
            .velocity_y = math::RandomInt() % 20 + 8};
  }
}

void StageVisuals::UpdateStars() {
  for (auto &star : fast_stars_) {
    star.y += star.velocity_y;
    if (star.y > 480) {
      star.x = math::RandomInt() % (640 - 256) + 128;
      star.y -= 480;
      star.velocity_y = math::RandomInt() % 20 + 8;
    }
  }
}

void StageVisuals::DrawStars() const {
  for (const auto &star : fast_stars_) {
    GrpSurface_Blit({star.x, star.y}, SURFACE_ID::MAPCHIP,
                    PIXEL_LTRB{624, 0, 640, 16});
  }
}

} // namespace stage
