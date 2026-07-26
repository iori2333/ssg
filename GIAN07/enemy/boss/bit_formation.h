///
/// BitFormation - orbiting boss-part state machine
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "boss.h"

#include "enemy/actor/enemy_actor.h"

struct BulletManager;
class EnemyManager;
class Player;

inline constexpr std::size_t BIT_MAX = 6;

struct BitLink {
  PIXEL_POINT from{};
  PIXEL_POINT to{};
};

struct BitLinkGeometry {
  std::array<BitLink, BIT_MAX> links{};
  std::size_t count = 0;
};

class BitFormation {
public:
  BitFormation(EnemyManager &enemies, BulletManager &bullets, Player &player)
      : enemies_(&enemies), bullets_(&bullets), player_(&player) {}

  void Reset();
  void Spawn(BossActor &parent, uint8_t count, uint32_t script_id);
  void Update();
  void Destroy();
  [[nodiscard]] BitLinkGeometry LinkGeometry() const;
  void SelectAttack(uint32_t script_id);
  void LaserCommand(EclBitLaserCommand command);
  void Command(EclBitCommand command, int parameter);
  void OnActorRetired(const EnemyActor &actor);
  [[nodiscard]] int Count() const;
  [[nodiscard]] bool Owns(const EnemyActor &actor) const {
    return motion_ != MotionState::Disabled && parent_ == &actor;
  }

private:
  enum class MotionState : uint8_t { Orbit, MoveTowardPlayer, Disabled = 0xff };
  enum class LaserPattern : uint8_t {
    Fixed = 3,
    Bidirectional = 4,
    Star = 5,
    Disabled = 0xff,
  };

  struct Part {
    EnemyActor *actor = nullptr;
    uint32_t hp = 0;
    uint8_t id = 0;
    uint8_t angle = 0;
    int8_t force = 0;
  };

  void UpdateRadius();
  void UpdateRotation();
  void PruneInvalidParts();

  std::array<Part, BIT_MAX> parts_{};
  EnemyActor *parent_ = nullptr;
  int center_x_ = 0;
  int center_y_ = 0;
  int speed_ = 0;
  int acceleration_ = 0;
  uint8_t direction_ = 64;
  uint8_t count_ = 0;
  int radius_ = 0;
  int target_radius_ = 0;
  int8_t rotation_speed_ = 0;
  MotionState motion_ = MotionState::Disabled;
  LaserPattern laser_pattern_ = LaserPattern::Disabled;
  uint16_t base_angle_ = 0;
  bool laser_active_ = false;

  EnemyManager *enemies_;
  BulletManager *bullets_;
  Player *player_;
};
