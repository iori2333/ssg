///
/// SnakeFormation - boss-linked segmented tail state
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "boss.h"

#include "enemy/actor/enemy_actor.h"

struct BulletManager;
class EnemyManager;

inline constexpr std::size_t kSnakeCapacity = 4;
inline constexpr std::size_t kSnakeLength = 30;
inline constexpr std::size_t kSnakePointsPerSegment = 8;

class SnakeFormation {
public:
  SnakeFormation(EnemyManager &enemies, BulletManager &bullets)
      : enemies_(enemies), bullets_(bullets) {}

  void Reset();
  void Spawn(BossActor &parent, uint32_t tail_script);
  void Update();
  void Remove(const BossActor &parent);
  void OnActorRetired(const EnemyActor &actor);

private:
  struct TrailPoint {
    int x{};
    int y{};
    uint8_t direction{};
  };

  struct Snake {
    std::array<TrailPoint, kSnakeLength * kSnakePointsPerSegment> trail{};
    std::array<EnemyActor *, kSnakeLength> segments{};
    EnemyActor *parent = nullptr;
    std::size_t head = 0;
    bool active = false;
  };

  void Destroy(Snake &snake);

  std::array<Snake, kSnakeCapacity> snakes_{};
  EnemyManager &enemies_;
  BulletManager &bullets_;
};
