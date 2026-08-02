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

namespace audio {
class AudioSystem;
}

inline constexpr int kBitCapacity = 6;

struct BitLink {
  PixelPoint from{};
  PixelPoint to{};
};

struct BitLinkGeometry {
  std::array<BitLink, kBitCapacity> links{};
  std::size_t count = 0;
};

class BitFormation {
public:
  BitFormation(EnemyManager &enemies, BulletManager &bullets, Player &player,
               audio::AudioSystem &audio)
      : enemies_(enemies), bullets_(bullets), player_(player), audio_(audio) {}

  void Reset();
  void Spawn(BossActor &parent, int count, uint32_t script_id);
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
  enum class MotionState : uint8_t {
    Orbit = 0,
    MoveTowardPlayer = 1,
    Disabled = 0xff
  };
  enum class LaserPattern : uint8_t {
    Fixed = 3,
    Bidirectional = 4,
    Star = 5,
    Disabled = 0xff,
  };

  struct Part {
    EnemyActor *actor = nullptr;
    uint32_t hp = 0;
    int id = 0;
    uint8_t angle = 0;
    int force = 0;
  };

  void UpdateRadius();
  void UpdateRotation();
  void PruneInvalidParts();

  std::array<Part, kBitCapacity> parts_{};
  EnemyActor *parent_ = nullptr;
  int center_x_ = 0;
  int center_y_ = 0;
  int speed_ = 0;
  int acceleration_ = 0;
  uint8_t direction_ = 64;
  int count_ = 0;
  int radius_ = 0;
  int target_radius_ = 0;
  int rotation_speed_ = 0;
  MotionState motion_ = MotionState::Disabled;
  LaserPattern laser_pattern_ = LaserPattern::Disabled;
  int base_angle_ = 0;
  bool laser_active_ = false;

  EnemyManager &enemies_;
  BulletManager &bullets_;
  Player &player_;
  audio::AudioSystem &audio_;
};
