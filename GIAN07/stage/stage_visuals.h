///
/// StageVisuals - specialized animated backgrounds owned by StageBackground.
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace stage {

enum class Stage4RockCommand : uint8_t {
  Normal = 0,
  Accelerate = 1,
  Reverse = 2,
  Rotate = 3,
  Leave = 4,
  End = 5,
};

class StageVisuals {
public:
  void StartCubes();
  void UpdateCubes();
  void DrawCubes() const;

  void StartFakeEcl();
  void UpdateFakeEcl();
  void DrawFakeEcl() const;

  void StartRocks();
  void UpdateRocks();
  void DrawRocks() const;
  void CommandRocks(Stage4RockCommand command);

  void StartRasters();
  void UpdateRasters();
  void DrawRasters() const;

  void StartStars();
  void UpdateStars();
  void DrawStars() const;

private:
  static constexpr std::size_t kCubeCount = 8;
  static constexpr std::size_t kCubeStarCount = 40;
  static constexpr std::size_t kFakeEclCount = 80;
  static constexpr std::size_t kRockCount = 28;
  static constexpr std::size_t kRasterCount = 28;
  static constexpr std::size_t kRasterStarCount = 60;
  static constexpr std::size_t kFastStarCount = 180;

  struct Point3D {
    int x = 0;
    int y = 0;
    int z = 0;
  };

  struct Rotation3D {
    int x = 0;
    int y = 0;
    int z = 0;
  };

  struct Cube {
    Point3D position;
    Rotation3D rotation;
    int half_size = 0;
  };

  struct Star {
    int x = 0;
    int y = 0;
    int velocity_y = 0;
  };

  struct FakeEclLine {
    int source_x = 0;
    int source_y = 0;
    int x = 0;
    int y = 0;
    int velocity_x = 0;
    int velocity_y = 0;
  };

  enum class RockState : uint8_t {
    Normal,
    Accelerating,
    Reversing,
    Leaving,
    Ending,
  };

  struct Rock {
    int x = 0;
    int y = 0;
    int velocity_y = 0;
    uint32_t age = 0;
    int speed = 0;
    int8_t acceleration = 0;
    uint8_t sprite = 0;
    RockState state = RockState::Normal;
  };

  struct Raster {
    int x = 0;
    int y = 0;
    int8_t velocity_y = 0;
    uint8_t type = 0;
    uint8_t angle = 0;
    uint8_t amplitude = 0;
  };

  static void Transform(Point3D &point, uint8_t angle_x, uint8_t angle_y,
                        uint8_t angle_z);
  static void DrawCube(const Cube &cube);

  std::array<Cube, kCubeCount> cubes_{};
  std::array<Star, kCubeStarCount> cube_stars_{};
  uint16_t cube_phase_ = 0;
  uint16_t cube_angle_x_ = 0;
  uint16_t cube_angle_y_ = 0;
  uint16_t cube_angle_z_ = 0;

  std::array<FakeEclLine, kFakeEclCount> fake_ecl_{};
  int grid_offset_x_ = 0;
  int grid_offset_y_ = 0;

  std::array<Rock, kRockCount> rocks_{};
  std::array<Raster, kRasterCount> rasters_{};
  std::array<Star, kRasterStarCount> raster_stars_{};
  std::array<Star, kFastStarCount> fast_stars_{};
};

} // namespace stage
