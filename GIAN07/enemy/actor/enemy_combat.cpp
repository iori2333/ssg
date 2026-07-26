///
/// Enemy combat query geometry
///

#include <algorithm>
#include <cstdlib>

#include "enemy_combat.h"

#include "core/gian.h"
#include "util/ut_math.h"

bool EnemyAttackHits(const EnemyActor &actor, const EnemyAttack &attack) {
  switch (attack.shape) {
  case EnemyAttackShape::Point:
    return HITCHK(attack.origin.x, actor.x, actor.hitbox_half_width) &&
           HITCHK(attack.origin.y, actor.y, actor.hitbox_half_height);

  case EnemyAttackShape::VerticalBeam:
    return HITCHK(attack.origin.x, actor.x, actor.hitbox_half_width) &&
           attack.origin.y > actor.y;

  case EnemyAttackShape::DirectedBeam: {
    const int hit_width =
        std::min(actor.hitbox_half_height, actor.hitbox_half_width) +
        PixelToWorld(3);
    const int offset_x = actor.x - attack.origin.x;
    const int offset_y = actor.y - attack.origin.y;
    const int length =
        cosl(attack.direction, offset_x) + sinl(attack.direction, offset_y);
    const int width = std::abs(-sinl(attack.direction, offset_x) +
                               cosl(attack.direction, offset_y));
    return length > 0 && width < hit_width;
  }

  case EnemyAttackShape::All:
    return true;
  }
  return false;
}
