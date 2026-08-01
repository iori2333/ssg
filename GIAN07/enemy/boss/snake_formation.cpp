///
/// Snake formation state machine
///

#include <algorithm>
#include <cstddef>

#include "snake_formation.h"

#include "bullet/bullet_manager.h"
#include "bullet/laser/long.h"
#include "enemy/enemy_manager.h"

void SnakeFormation::Reset() {
  for (auto &snake : snakes_) {
    snake = {};
  }
}

void SnakeFormation::Spawn(BossActor &parent, uint32_t tail_script) {
  auto snake = std::ranges::find_if(
      snakes_, [](const auto &candidate) { return !candidate.active; });
  if (snake == snakes_.end()) {
    return;
  }

  snake->active = true;
  snake->parent = &parent;
  snake->head = 0;

  for (auto &point : snake->trail) {
    point = {.x = parent.x, .y = parent.y, .direction = parent.d};
  }

  const WorldPoint position{&parent.x, &parent.y};
  for (auto &segment : snake->segments) {
    if (auto *actor = enemies_.SpawnRegular(position, tail_script)) {
      segment = actor;
    }
  }
}

void SnakeFormation::Update() {
  constexpr auto point_count = kSnakeLength * kSnakePointsPerSegment;

  for (auto &snake : snakes_) {
    if (!snake.active) {
      continue;
    }

    const auto *parent = snake.parent;
    if (parent == nullptr) {
      Destroy(snake);
      continue;
    }

    for (std::size_t index = 0; index < snake.segments.size(); ++index) {
      auto *segment = snake.segments[index];
      if (segment == nullptr) {
        continue;
      }

      const auto trail_index =
          (snake.head + point_count - index * kSnakePointsPerSegment) %
          point_count;
      const auto &point = snake.trail[trail_index];
      segment->x = point.x;
      segment->y = point.y;
      segment->d = point.direction;
    }

    snake.head = (snake.head + 1) % point_count;
    snake.trail[snake.head] = {
        .x = parent->x,
        .y = parent->y,
        .direction = parent->d,
    };
  }
}

void SnakeFormation::Remove(const BossActor &parent) {
  auto snake = std::ranges::find_if(snakes_, [&parent](const auto &candidate) {
    return candidate.parent == &parent;
  });
  if (snake == snakes_.end()) {
    return;
  }

  Destroy(*snake);
}

void SnakeFormation::OnActorRetired(const EnemyActor &actor) {
  for (auto &snake : snakes_) {
    if (snake.parent == &actor) {
      Destroy(snake);
      continue;
    }
    for (auto &segment : snake.segments) {
      if (segment == &actor) {
        segment = nullptr;
      }
    }
  }
}

void SnakeFormation::Destroy(Snake &snake) {
  for (auto *segment : snake.segments) {
    if (segment == nullptr) {
      continue;
    }

    if (segment->long_laser_count != 0U) {
      bullets_.ControlLongLaser(
          segment, kEclAllLongLasers,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
    }
    segment->BeginExplosion();
  }

  snake = {};
}
