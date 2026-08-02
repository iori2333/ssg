/// Enemy bullet entity state and movement.

#pragma once

#include <cstdint>

#include "bullet_common.h"
#include "fire_state.h"

#include "gfx/coords.h"
#include "util/enum_flags.h"

struct GameSession;

inline constexpr auto kBulletGrazeValue = 1;
inline constexpr auto kBulletClearScoreStart = 10000;
inline constexpr auto kBulletClearScoreEnd = 15000;
inline constexpr auto kSmallBulletGrazeRadius = 24_px;
inline constexpr auto kLargeBulletGrazeRadius = 32_px;

inline constexpr auto kBulletVisualCategoryMask = 0xf0;
inline constexpr auto kSmallBulletVisual = 0x00;
inline constexpr auto kLargeBulletVisual = 0x10;
inline constexpr auto kDirectionalBulletVisual = 0x20;
inline constexpr auto kExtraBulletVisual = 0x30;
inline constexpr auto kLargeExtraBulletVisual = 0x40;
inline constexpr auto kSpecialDirectionalBulletVisual = 0x25;
inline constexpr int kSmallBulletHitRadius = 2.5_px;
inline constexpr int kMediumBulletHitRadius = 4.5_px;
inline constexpr int kLargeBulletHitRadius = 7.5_px;
inline constexpr int kExtraLargeBulletHitRadius = 10.5_px;

int GetBulletHitRadius(uint8_t c);
int GetBulletEvadeRadius(uint8_t c);

inline constexpr auto kBulletCapacity = 2048;

enum class BulletSpawnType : uint8_t {
  Normal = 0x00,
  Line = 0x01,
  Extra01 = 0x02,
};

enum class BulletMotion : uint8_t {
  Normal,
  Accelerating,
  Retargeting,
  Homing,
  Turning,
  TurningAccelerating,
  TurningReversing,
  Gravity,
  ChangeDirection,
  SpecialHoming,
  Bomb,
};

enum class BulletOptionKind : uint8_t {
  None,
  Wave,
  Orbit,
  Stationary,
  ReflectX,
  ReflectY,
  ReflectXY,
  Divide,
  Bomb,
};

enum class BulletEffect : uint8_t {
  None,
  Roll1,
  Roll2,
  Warning,
  Rock,
  Circle1,
  Circle2,
  Clearing,
};

enum class BulletSpeedVariance : uint8_t { None, Small, Medium, Large };

struct BulletSpawnInfo {
  int x{};
  int y{};
  float speed{};
  float acceleration{};
  float angle{};
  int spread{};
  int count{};
  int rapid_count{};
  uint8_t visual{};
  BulletSpeedVariance speed_variance{BulletSpeedVariance::None};
  BulletOptionKind option{BulletOptionKind::None};
  int option_count{};
  BulletMotion motion{BulletMotion::Normal};
  int repeat{};
  int8_t angular_velocity{};
  BulletEffect effect{BulletEffect::None};
  BulletPattern pattern{BulletPattern::Spread};
  bool rapid{};
  bool aimed{};
  BulletSpawnType spawn_type{BulletSpawnType::Normal};
};

struct BulletManager;

struct BulletUpdateInfo {
  int player_x, player_y;
  bool enemy_homing_valid;
  int enemy_homing_x, enemy_homing_y;

  struct UpdateResult {
    bool smoke_spawn = false;
    int smoke_x = 0, smoke_y = 0;
    bool division_requested = false;
    BulletSpawnInfo division_info;
    int division_cx = 0, division_cy = 0;
  };
};

struct Bullet {
  using SpawnInfo = BulletSpawnInfo;
  using UpdateInfo = BulletUpdateInfo;
  using UpdateResult = UpdateInfo::UpdateResult;

  void Render() const;
  [[nodiscard]] bool IsDead() const;
  void Kill();
  void Spawn(const BulletSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

  [[nodiscard]] int X() const;
  [[nodiscard]] int Y() const;

  [[nodiscard]] bool IsSmall() const;
  [[nodiscard]] bool IsClearing() const;
  [[nodiscard]] bool RegisterGraze();
  void RemoveImmediately();
  void UpdateDisplayAngle();

private:
  enum class Flags : uint8_t {
    None = 0,
    KeepOutsidePlayfield = 1 << 0,
    Grazed = 1 << 1,
    PendingRemoval = 1 << 7,
  };

  [[nodiscard]] bool HasFlag(Flags flag) const {
    return (std::to_underlying(flags_) & std::to_underlying(flag)) != 0;
  }
  void SetFlag(Flags flag, bool enabled) {
    const auto bits = std::to_underlying(flags_);
    const auto flag_bit = std::to_underlying(flag);
    flags_ = static_cast<Flags>(enabled ? bits | flag_bit : bits & ~flag_bit);
  }

  float x_{};
  float y_{};
  float v_{};
  float angle_{};
  uint8_t c_{};
  int count_{};

  float tx_{};
  float ty_{};
  float vx_{};
  float vy_{};
  float v0_{};
  float a_{};
  int8_t vd_{};
  int rep_{};
  BulletMotion motion_{BulletMotion::Normal};
  BulletOptionKind option_{BulletOptionKind::None};
  int option_count_{};
  BulletEffect effect_{BulletEffect::None};
  Flags flags_ = Flags::None;

  void MoveByType(const UpdateInfo &info, UpdateResult &result);
  void MoveByOption(UpdateResult &result);
  void MoveByEffect();
  void RevertToNormal();
  void DrawEffect() const;
  [[nodiscard]] uint8_t DisplayAngle() const;
};

[[nodiscard]] BulletSpawnInfo
MakeBulletSpawnInfo(const EclBulletState &cmd, int ox, int oy, bool scaling,
                    const GameSession &game,
                    BulletSpawnType spawn_type = BulletSpawnType::Normal);
void ScaleBulletSpawnInfo(BulletSpawnInfo &info, const GameSession &game);
