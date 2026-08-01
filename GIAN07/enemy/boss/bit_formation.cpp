///
/// Bit formation state machine
///

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

#include "bit_formation.h"

#include "audio/sfx.h"
#include "bullet/bullet_manager.h"
#include "bullet/laser/long.h"
#include "enemy/enemy_manager.h"
#include "player/player.h"
#include "util/math_utils.h"

static constexpr auto kBitVirtualHp = 990000;

// Initialize bit array
void BitFormation::Reset() {
  center_x_ = 0;
  center_y_ = 0;
  base_angle_ = 0;
  radius_ = 0;
  target_radius_ = 0;
  speed_ = 0;
  acceleration_ = 0;
  direction_ = 64;
  rotation_speed_ = 0;
  count_ = 0;
  laser_pattern_ = LaserPattern::Disabled;
  laser_active_ = false;
  motion_ = MotionState::Disabled;
  parent_ = nullptr;

  parts_ = {};
  for (size_t index = 0; index < parts_.size(); ++index) {
    parts_[index].id = static_cast<uint8_t>(index);
  }
}

// Set bits
void BitFormation::Spawn(BossActor &parent, uint8_t count, uint32_t script_id) {
  static constexpr std::array<uint8_t, kBitCapacity> hp_multipliers = {1, 4, 2,
                                                                       5, 3, 6};

  int i = 0;

  // Unlike other functions, note the inequality
  // If this bit structure is active, this function cannot execute
  // So return immediately
  if (motion_ != MotionState::Disabled) {
    return;
  }

  // Invalid bit count
  if (count == 0 || count > kBitCapacity) {
    return;
  }

  motion_ = MotionState::Orbit;
  parent_ = &parent;

  center_x_ = parent.x;
  center_y_ = parent.y;

  radius_ = 0;
  target_radius_ = 80_px;
  count_ = count;
  rotation_speed_ = (((math::RandomInt() >> 1) & 1) != 0) ? 2 : -2;
  base_angle_ = 0;
  laser_pattern_ = LaserPattern::Disabled;
  laser_active_ = false;
  const WorldPoint position{&center_x_, &center_y_};
  for (i = 0; std::cmp_less(i, count); i++) {
    if (auto *e = enemies_.SpawnRegular(position, script_id)) {
      e->hp = kBitVirtualHp;
      e->d = i * (256 / count);
      e->script.registers[0] = i;
      e->script.registers[1] = count;
      enemies_.ecl_.Execute(*e);

      // Associate this structure with the created enemy
      parts_[i].actor = e;
      parts_[i].angle = e->d; // Current angle
      parts_[i].force = 0;    // Other force direction
      parts_[i].id = i;       // Bit index from start

      parts_[i].hp = 95 * hp_multipliers[i];
    } else {
      parts_[i].actor = {};
    }
  }
}

// Move bits
void BitFormation::Update() {
  int i = 0;
  int j = 0;
  EnemyActor *e = nullptr;

  if (count_ == 0) {
    return;
  }

  PruneInvalidParts();
  if (count_ == 0) {
    return;
  }

  switch (motion_) {
  case MotionState::Orbit: {
    const auto *parent = parent_;
    if (parent == nullptr) {
      Destroy();
      return;
    }
    center_x_ = parent->x;
    center_y_ = parent->y;

    UpdateRadius();
    UpdateRotation();
    break;
  }

  case MotionState::MoveTowardPlayer: {
    speed_ += acceleration_;
    const auto velocity =
        math::RoundedPolarVector(math::AngleFromLegacy(direction_), speed_);
    center_x_ += velocity.x;
    center_y_ += velocity.y;

    if (speed_ <= -10_px) {
      motion_ = MotionState::Orbit;
    }

    UpdateRadius();
    UpdateRotation();
    break;
  }

  case MotionState::Disabled:
  default:
    return;
  }

  // Damage is nullified during laser emission
  if (laser_active_) {
    // Restore enemy HP to virtual HP
    // -> To accumulate damage, comment out the for loop below
    for (i = 0; std::cmp_less(i, count_); i++) {
      if (auto *actor = parts_[i].actor) {
        actor->hp = kBitVirtualHp;
      }
    }
    return;
  }

  // Note: count_ decreases when bits are removed
  for (i = 0; std::cmp_less(i, count_); i++) {
    e = parts_[i].actor;
    if (e == nullptr) {
      continue;
    }

    const uint32_t damage = (kBitVirtualHp - e->hp);
    if (parts_[i].hp <= damage) {
      // Send deletion request to enemy associated with bit array
      if (e->long_laser_count != 0U) {
        bullets_.ControlLongLaser(
            e, kEclAllLongLasers,
            LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
      }
      e->BeginExplosion();

      PlaySfx(SfxId::Bomb, e->x);

      for (j = i + 1; std::cmp_less(j, count_); j++) {
        parts_[j - 1] = parts_[j];
        parts_[j - 1].id--;
      }

      // When the bit at the base angle is destroyed
      if (i == 0) {
        base_angle_ += (256 / count_);
      }

      // Decrease total bit count
      count_--;

      // Transition to bit disabled state
      if (count_ == 0) {
        motion_ = MotionState::Disabled;
      }

      // Apply force before and after the destroyed bit
      // Note: count is already decremented at this point
      if (count_ != 0U) {
        j = i - 1 + count_;
        parts_[j % count_].force -= 30;
        parts_[i % count_].force += 30;
      }
      // The bit was erased, so the next data is now at index i
      // Therefore, decrement i to move to the next bit reference
      //
      i--;
    } else {
      // Restore enemy HP to virtual HP
      e->hp = kBitVirtualHp;

      // Where actual damage is applied
      parts_[i].hp -= damage;
    }
  }

  // Update registers
  for (i = 0; std::cmp_less(i, count_); i++) {
    e = parts_[i].actor;
    if (e == nullptr) {
      continue;
    }
    e->script.registers[1] = count_;
  }
}

void BitFormation::PruneInvalidParts() {
  std::size_t output = 0;
  for (std::size_t input = 0; input < count_; ++input) {
    if (parts_[input].actor == nullptr) {
      continue;
    }
    if (output != input) {
      parts_[output] = parts_[input];
    }
    parts_[output].id = static_cast<uint8_t>(output);
    ++output;
  }
  count_ = static_cast<uint8_t>(output);
  if (count_ == 0) {
    Reset();
  }
}

// Basic radius processing
void BitFormation::UpdateRadius() {
  if (radius_ > target_radius_) {
    radius_ -= 2_px;

    radius_ = std::max(radius_, target_radius_);
  } else if (radius_ < target_radius_) {
    radius_ += 2_px;

    radius_ = std::min(radius_, target_radius_);
  }
}

// Basic bit rotation processing
void BitFormation::UpdateRotation() {
  int i = 0;
  int ox = 0;
  int oy = 0;
  int n = 0;
  int l = 0;

  int dir = 0;
  uint8_t LaserDeg = 0;

  EnemyActor *e = nullptr;
  Part *bit = nullptr;

  if (count_ == 0) {
    return;
  }

  base_angle_ += rotation_speed_;

  n = count_;
  l = radius_;

  ox = center_x_;
  oy = center_y_;

  const int delta = (256 / count_);
  const int ExSpeed = abs(rotation_speed_ / 2);

  // d       : Target angle for the bit
  // delta   : Ideal angle between bits (convergence angle)
  // ExSpeed : Absolute rotation speed of the bit + 1
  for (i = 0; i < n; i++) {
    bit = parts_.data() + i;
    e = bit->actor;
    if (e == nullptr) {
      continue;
    }

    // Find target angle
    const uint8_t d = ((base_angle_ >> 1) + (delta * bit->id));

    // Normal angle convergence processing
    dir = (static_cast<int>(d) - static_cast<int>(bit->angle));

    if (dir < -128) {
      dir += 256;
    } else if (dir > 128) {
      dir -= 256;
    }

    if (dir > 0) {
      dir = std::min(dir, 2);
      if (rotation_speed_ > 0) {
        bit->angle += std::max(dir, ExSpeed);
      } else {
        bit->angle += std::max(dir, (ExSpeed + 1));
      }
    } else if (dir < 0) {
      dir = std::max(dir, -2);
      if (rotation_speed_ < 0) {
        bit->angle -= std::max(-dir, ExSpeed);
      } else {
        bit->angle -= std::max(-dir, (ExSpeed + 1));
      }
    }

    // Reflect force influence
    if (bit->force > 0) {
      bit->force--;
      if (rotation_speed_ > 0) {
        bit->angle++;
      } else {
        bit->angle += (ExSpeed + 1);
      }
      // Sleep(100);
    } else if (bit->force < 0) {
      bit->force++;
      if (rotation_speed_ < 0) {
        bit->angle--;
      } else {
        bit->angle -= (ExSpeed + 1);
      }
      // Sleep(100);
    }

    e->d = bit->angle;
    const auto offset =
        math::RoundedPolarVector(math::AngleFromLegacy(e->d), l);
    e->x = ox + offset.x;
    e->y = oy + offset.y;

    // Reflect laser command
    switch (laser_pattern_) {
    case LaserPattern::Fixed:         // Emit unidirectional fixed-angle laser
    case LaserPattern::Bidirectional: // Emit bidirectional fixed-angle laser
      break;

    case LaserPattern::Star: // Angle-synchronized n-point star laser
      if (count_ == 0) {
        break;
      }
      LaserDeg = 64 + (256 / count_);
      bullets_.ControlLongLaser(
          e, 0,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::SetAngle,
              math::AngleFromLegacy(static_cast<uint8_t>(e->d + LaserDeg))});
      bullets_.ControlLongLaser(
          e, 1,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::SetAngle,
              math::AngleFromLegacy(static_cast<uint8_t>(e->d - LaserDeg))});
      break;
    case LaserPattern::Disabled:
      break;
    }
  }
}

// Destroy bits
void BitFormation::Destroy() {
  int i = 0;
  EnemyActor *e = nullptr;

  if (motion_ == MotionState::Disabled) {
    return;
  }

  // Destroy each bit
  for (i = 0; std::cmp_less(i, count_); i++) {
    e = parts_[i].actor;
    if (e == nullptr) {
      continue;
    }

    if (e->long_laser_count != 0U) {
      bullets_.ControlLongLaser(
          e, kEclAllLongLasers,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
    }
    e->BeginExplosion();

    PlaySfx(SfxId::Bomb, e->x);
  }

  // Delegate the rest to this function
  Reset();
}

BitLinkGeometry BitFormation::LinkGeometry() const {
  BitLinkGeometry geometry;
  std::array<EnemyActor *, kBitCapacity * 2> references{};

  if (motion_ == MotionState::Disabled) {
    return geometry;
  }

  size_t actor_count = 0;
  for (const auto &part : parts_) {
    if (auto *actor = part.actor; actor != nullptr && actor_count < count_) {
      references[actor_count++] = actor;
    }
  }
  if (actor_count == 0) {
    return geometry;
  }
  for (size_t index = 0; index < actor_count; ++index) {
    references[index + actor_count] = references[index];
  }

  for (size_t index = 0; index < actor_count; ++index) {
    const auto next = index + (actor_count >= 5 ? 2 : 1);
    geometry.links[geometry.count++] = {
        .from =
            WorldPoint::FromWorld(references[index]->x, references[index]->y)
                .ToPixel(),
        .to = WorldPoint::FromWorld(references[next]->x, references[next]->y)
                  .ToPixel(),
    };
  }
  return geometry;
}

// Set or change attack pattern
void BitFormation::SelectAttack(uint32_t script_id) {
  int i = 0;

  for (i = 0; std::cmp_less(i, count_); i++) {
    if (auto *actor = parts_[i].actor) {
      if (!enemies_.ecl_.Jump(*actor, script_id)) {
        actor->state = EnemyActorState::PendingRemoval;
      }
    }
  }
}

// Issue laser commands
void BitFormation::LaserCommand(EclBitLaserCommand command) {
  int i = 0;
  EnemyActor *e = nullptr;
  uint8_t delta = 0;

  laser_active_ = true;

  for (i = 0; std::cmp_less(i, count_); i++) {
    e = parts_[i].actor;
    if (e == nullptr) {
      continue;
    }

    LongLaserSpawnInfo info{
        .enemy = e,
        .dx = 0,
        .dy = 0,
        .v = 1_px,
        .w = 8_px,
        .angle = math::AngleFromLegacy(e->d),
        .type = LongLaserType::Long,
    };

    switch (command) {
    case EclBitLaserCommand::Fixed:
      info.c = 2;
      info.enemy_id = e->long_laser_count;
      if (bullets_.SpawnLongLaser(info)) {
        e->long_laser_count++;
      }
      break;

    case EclBitLaserCommand::Bidirectional:
      info.angle += math::kFullAngle / 4.0f;
      info.c = 1;
      info.enemy_id = e->long_laser_count;
      if (bullets_.SpawnLongLaser(info)) {
        e->long_laser_count++;
      }

      info.angle += math::kFullAngle / 2.0f;
      info.enemy_id = e->long_laser_count;
      if (bullets_.SpawnLongLaser(info)) {
        e->long_laser_count++;
      }
      break;

    case EclBitLaserCommand::Star:
      info.c = 0;

      delta = 64 + (256 / count_);

      info.angle = math::AngleFromLegacy(static_cast<uint8_t>(e->d + delta));
      info.enemy_id = e->long_laser_count;
      if (bullets_.SpawnLongLaser(info)) {
        e->long_laser_count++;
      }
      info.angle = math::AngleFromLegacy(static_cast<uint8_t>(e->d - delta));
      info.enemy_id = e->long_laser_count;
      if (bullets_.SpawnLongLaser(info)) {
        e->long_laser_count++;
      }
      break;

    case EclBitLaserCommand::Open:
      bullets_.ControlLongLaser(
          e, kEclAllLongLasers,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::Open});
      continue;

    case EclBitLaserCommand::Close:
      bullets_.ControlLongLaser(
          e, kEclAllLongLasers,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::Close});
      e->long_laser_count = 0;
      laser_active_ = false;
      continue;

    case EclBitLaserCommand::CloseToLine:
      bullets_.ControlLongLaser(
          e, kEclAllLongLasers,
          LongLaserUpdateInfo{LongLaserUpdateInfo::Command::CloseToLine});
      continue;
    }

    laser_pattern_ = static_cast<LaserPattern>(static_cast<uint8_t>(command));
  }
}

// Send bit command
void BitFormation::Command(EclBitCommand command, int param) {
  switch (command) {
  case EclBitCommand::ChangeSpeed:
    // Change speed in the same direction
    if (param > 0) {
      if (rotation_speed_ > 0) {
        rotation_speed_ = static_cast<int8_t>(param);
      } else {
        rotation_speed_ = static_cast<int8_t>(-param);
      }
    }
    // Reverse rotation direction and change speed
    else {
      if (rotation_speed_ > 0) {
        rotation_speed_ = static_cast<int8_t>(param);
      } else {
        rotation_speed_ = static_cast<int8_t>(-param);
      }
    }
    break;

  case EclBitCommand::SelectAttack:
    SelectAttack(static_cast<uint32_t>(param));
    break;

  case EclBitCommand::ChangeRadius:
    target_radius_ = param;
    break;

  case EclBitCommand::MoveTowardPlayer:
    speed_ = 10_px;
    acceleration_ = -8;
    direction_ = math::AngleToLegacy(
        math::AngleTo(static_cast<float>(player_.X() - center_x_),
                      static_cast<float>(player_.Y() - center_y_)));
    motion_ = MotionState::MoveTowardPlayer;
    break;

  default:
    return;
  }
}

// Get current bit count
int BitFormation::Count() const {
  if (motion_ == MotionState::Disabled) {
    return 0;
  }
  return count_;
}

void BitFormation::OnActorRetired(const EnemyActor &actor) {
  if (parent_ == &actor) {
    Destroy();
    return;
  }
  for (auto &part : parts_) {
    if (part.actor == &actor) {
      part.actor = nullptr;
    }
  }
}
