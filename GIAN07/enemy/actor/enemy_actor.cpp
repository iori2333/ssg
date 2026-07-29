///
/// EnemyActor - local state transitions, animation, and hit geometry
///

#include <algorithm>
#include <cstdlib>

#include "enemy_actor.h"

#include "gameplay/playfield.h"
#include "player/player_attack.h"
#include "util/ut_math.h"

void EnemyActor::BeginExplosion() {
  hp = 0;
  count = 0;
  state = EnemyActorState::Exploding;
}

void EnemyActor::UpdateAnimation(const EnemyAnimationSet &animations) {
  if (animation >= animations.size()) {
    state = EnemyActorState::PendingRemoval;
    return;
  }
  const auto &animation_data = animations[animation];
  if (animation_data.n == 0) {
    state = EnemyActorState::PendingRemoval;
    return;
  }

  switch (animation_data.mode) {
  case EnemyAnimationMode::Loop:
    if (animation_speed > 0 && count % animation_speed == 0) {
      animation_frame = (animation_frame + 1) % animation_data.n;
    } else if (animation_speed < 0 && count % -animation_speed == 0) {
      animation_frame =
          (animation_frame + animation_data.n - 1) % animation_data.n;
    }
    break;

  case EnemyAnimationMode::StopAtEnd:
    if (animation_speed > 0 && count % animation_speed == 0 &&
        animation_frame < animation_data.n - 1) {
      ++animation_frame;
    }
    break;

  case EnemyAnimationMode::Directional:
    break;
  }
}

bool EnemyActor::IsHitBy(const PlayerAttack &attack) const {
  switch (attack.shape) {
  case PlayerAttackShape::Point:
    return playfield::WithinAxisDistance(attack.origin.x, x,
                                         hitbox_half_width) &&
           playfield::WithinAxisDistance(attack.origin.y, y,
                                         hitbox_half_height);

  case PlayerAttackShape::VerticalBeam:
    return playfield::WithinAxisDistance(attack.origin.x, x,
                                         hitbox_half_width) &&
           attack.origin.y > y;

  case PlayerAttackShape::DirectedBeam: {
    const int hit_width =
        std::min(hitbox_half_height, hitbox_half_width) + PixelToWorld(3);
    const int offset_x = x - attack.origin.x;
    const int offset_y = y - attack.origin.y;
    const int length =
        cosl(attack.direction, offset_x) + sinl(attack.direction, offset_y);
    const int width = std::abs(-sinl(attack.direction, offset_x) +
                               cosl(attack.direction, offset_y));
    return length > 0 && width < hit_width;
  }

  case PlayerAttackShape::AllEnemies:
    return true;
  }
  return false;
}
