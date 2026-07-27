///
/// Bullet - Definitions and various things related to bullets
///

#pragma once

#include <cstdint>

#include "bullet_common.h"
#include "fire_state.h"

#include "gfx/coords.h"

struct GameSession;

////Bullet constants////
inline constexpr auto TAMA_EVADE = 1;

inline constexpr auto TAMA1_POINT = 10000;
inline constexpr auto TAMA2_POINT = 15000;

inline constexpr auto TAMA_EVADE_RADIUS_SMALL = 24_px;
inline constexpr auto TAMA_EVADE_RADIUS_LARGE = 32_px;

inline constexpr auto TAMA_SMALL = 0x00;
inline constexpr auto TAMA_LARGE = 0x10;
inline constexpr auto TAMA_ANGLE = 0x20;
inline constexpr auto TAMA_EXTRA = 0x30;
inline constexpr auto TAMA_EXTRA2 = 0x40;
inline constexpr auto TAMA_REN = 0x04;
inline constexpr auto TAMA_ZSET = 0x08;
inline constexpr int TAMA_HIT_S = 2.5_px;
inline constexpr int TAMA_HIT_M = 4.5_px;
inline constexpr int TAMA_HIT_L = 7.5_px;
inline constexpr int TAMA_HIT_XL = 10.5_px;

inline constexpr auto TF_NONE = 0x00;
inline constexpr auto TF_CLIP = 0x01;
inline constexpr auto TF_EVADE = 0x02;
inline constexpr auto TF_DELETE = 0x80;

int GetBulletHitRadius(uint8_t c);
int GetBulletEvadeRadius(uint8_t c);

////Pool capacities////
inline constexpr auto kBulletMax = 2048;

////Bullet spawn type discriminator////
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

////Spawn parameter struct////
struct BulletSpawnInfo {
  int x{};
  int y{};
  int speed{};
  int8_t acceleration{};
  uint8_t angle{};
  uint8_t spread{};
  uint8_t count{};
  uint8_t rapid_count{};
  uint8_t visual{};
  BulletSpeedVariance speed_variance{BulletSpeedVariance::None};
  BulletOptionKind option{BulletOptionKind::None};
  uint8_t option_count{};
  BulletMotion motion{BulletMotion::Normal};
  uint8_t repeat{};
  int8_t angular_velocity{};
  BulletEffect effect{BulletEffect::None};
  BulletPattern pattern{BulletPattern::Spread};
  bool rapid{};
  bool aimed{};
  BulletSpawnType spawn_type{BulletSpawnType::Normal};
};

struct BulletManager;

////World context + side-effect result (passed to / returned from
/// Bullet::Update)////
struct BulletUpdateInfo {
  int player_x, player_y;
  bool enemy_homing_valid;
  int enemy_homing_x, enemy_homing_y;

  struct UpdateResult {
    bool smoke_spawn = false;
    int smoke_x = 0, smoke_y = 0;
    bool division_requested = false;
    EclBulletState division_cmd;
    int division_cx = 0, division_cy = 0;
  };
};

////Bullet class////
struct Bullet {
  using SpawnInfo = BulletSpawnInfo;
  using UpdateInfo = BulletUpdateInfo;
  using UpdateResult = UpdateInfo::UpdateResult;

  void Render() const;
  bool IsDead() const;
  void Kill();
  void Spawn(const BulletSpawnInfo &info);
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y,
                                   int player_radius) const;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {});
  void RenderDebugHitbox(int mode) const;

  [[nodiscard]] int X() const { return x_; }
  [[nodiscard]] int Y() const { return y_; }

  [[nodiscard]] bool IsSmall() const;
  [[nodiscard]] bool IsClearing() const;
  [[nodiscard]] bool RegisterGraze();
  void RemoveImmediately();
  void UpdateDisplayAngle();

private:
  // ── Manager-internal ──────────────────────────────────────

  // ── Fields ────────────────────────────────────────────────
  int x_{};
  int y_{};
  int v_{};
  uint8_t d_{};
  uint8_t c_{};
  uint32_t count_{};

  int tx_{};
  int ty_{};
  int vx_{};
  int vy_{};
  int v0_{};
  int a_{};
  uint16_t d16_{};
  int8_t vd_{};
  uint8_t rep_{};
  BulletMotion motion_{BulletMotion::Normal};
  BulletOptionKind option_{BulletOptionKind::None};
  uint8_t option_count_{};
  BulletEffect effect_{BulletEffect::None};
  uint8_t flag_{};

  void MoveByType(const UpdateInfo &info, UpdateResult &result);
  void MoveByOption(UpdateResult &result);
  void MoveByEffect();
  void RevertToNormal();
  void DrawEffect() const;
};

//// Free function: build SpawnInfo from ECL command ////
[[nodiscard]] BulletSpawnInfo
MakeBulletSpawnInfo(const EclBulletState &cmd, int ox, int oy, bool scaling,
                    const GameSession &game,
                    BulletSpawnType spawn_type = BulletSpawnType::Normal);
