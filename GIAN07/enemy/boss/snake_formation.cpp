///
/// Snake formation state machine
///

#include <algorithm>
#include <cstddef>

#include "snake_formation.h"

#include "bullet/bullet_manager.h"
#include "bullet/laser/long.h"
#include "enemy/enemy_system.h"

void SnakeFormation::Reset() {
  for (auto &snake : snakes_) {
    snake = {};
  }
}

void SnakeFormation::Spawn(BossData &parent, uint32_t tail_script) {
  auto snake = std::ranges::find_if(
      snakes_, [](const auto &candidate) { return !candidate.active; });
  if (snake == snakes_.end()) {
    return;
  }

  snake->active = true;
  snake->parent = &parent.actor;
  snake->head = 0;

  for (auto &point : snake->trail) {
    point = {.x = parent.actor.x, .y = parent.actor.y, .d = parent.actor.d};
  }

  const WORLD_POINT position{&parent.actor.x, &parent.actor.y};
  for (auto &segment : snake->segments) {
    if (auto *actor = enemies_->SpawnRegular(position, tail_script)) {
      segment = actor;
    }
  }
}

void SnakeFormation::Update() {
  constexpr auto point_count = SNAKE_LENGTH * SNAKE_POINTS_PER_SEGMENT;

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
          (snake.head + point_count - index * SNAKE_POINTS_PER_SEGMENT) %
          point_count;
      const auto &point = snake.trail[trail_index];
      segment->x = point.x;
      segment->y = point.y;
      segment->d = point.d;
    }

    snake.head = (snake.head + 1) % point_count;
    snake.trail[snake.head] = {
        .x = parent->x,
        .y = parent->y,
        .d = parent->d,
    };
  }
}

void SnakeFormation::Remove(const BossData &parent) {
  auto snake = std::ranges::find_if(snakes_, [&parent](const auto &candidate) {
    return candidate.parent == &parent.actor;
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
      bullets_->ControlLongLaser(
          segment, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
    }
    segment->hp = 0;
    segment->count = 0;
    segment->state = EnemyActorState::Exploding;
  }

  snake = {};
}
