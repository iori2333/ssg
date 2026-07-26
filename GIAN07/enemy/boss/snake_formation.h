///
/// SnakeFormation - boss-linked segmented tail state
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "boss.h"

#include "core/point.h"
#include "enemy/actor/enemy_actor.h"

struct BulletManager;
class EnemySystem;

inline constexpr std::size_t SNAKE_MAX = 4;
inline constexpr std::size_t SNAKE_LENGTH = 30;
inline constexpr std::size_t SNAKE_POINTS_PER_SEGMENT = 8;

class SnakeFormation {
public:
  SnakeFormation(EnemySystem &enemies, BulletManager &bullets)
      : enemies_(&enemies), bullets_(&bullets) {}

  void Reset();
  void Spawn(BossData &parent, uint32_t tail_script);
  void Update();
  void Remove(const BossData &parent);
  void OnActorRetired(const EnemyActor &actor);

private:
  struct Snake {
    std::array<DegPoint, SNAKE_LENGTH * SNAKE_POINTS_PER_SEGMENT> trail{};
    std::array<EnemyActor *, SNAKE_LENGTH> segments{};
    EnemyActor *parent = nullptr;
    std::size_t head = 0;
    bool active = false;
  };

  void Destroy(Snake &snake);

  std::array<Snake, SNAKE_MAX> snakes_{};
  EnemySystem *enemies_;
  BulletManager *bullets_;
};
