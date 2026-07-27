///
/// PlayerLoadout - Static player traits and loadout-specific behavior.
///

#pragma once

#include <cstdint>

class Player;
class EnemyManager;
class EffectManager;

enum class PlayerType : uint8_t { Wide, Homing, Laser };

struct PlayerTraits {
  PlayerType type;
  int move_speed;
  int focus_move_speed;
  int hit_radius;
  uint16_t bomb_duration;
  uint8_t option_sprite;
  int option_offset;
  int focus_option_offset;
};

class PlayerLoadout {
public:
  virtual ~PlayerLoadout() = default;

  [[nodiscard]] PlayerType Type() const { return traits_.type; }
  [[nodiscard]] int MoveSpeed(bool focused) const {
    return focused ? traits_.focus_move_speed : traits_.move_speed;
  }
  [[nodiscard]] int HitRadius() const { return traits_.hit_radius; }
  [[nodiscard]] uint16_t BombDuration() const { return traits_.bomb_duration; }
  [[nodiscard]] uint8_t OptionSprite() const { return traits_.option_sprite; }
  [[nodiscard]] int OptionOffset(bool focused) const {
    return focused ? traits_.focus_option_offset : traits_.option_offset;
  }

  virtual void FireMain(Player &player, uint8_t tier, bool focused) = 0;
  virtual void FireSub(Player &player, uint8_t tier, bool focused) = 0;
  virtual void UpdateBomb(Player & /*player*/, EnemyManager & /*enemies*/,
                          EffectManager & /*effects*/, uint16_t /*remaining*/) {
  }
  virtual void Tick(Player & /*player*/) {}
  virtual void ApplyContinuousAttack(const Player & /*player*/,
                                     EnemyManager & /*enemies*/,
                                     bool /*focused*/) const {}
  virtual void DrawBombBackground(const Player & /*player*/,
                                  uint16_t /*remaining*/) const {}
  virtual void DrawBombForeground(const Player & /*player*/,
                                  uint16_t /*remaining*/) const {}
  virtual void DrawContinuousAttack(const Player & /*player*/,
                                    bool /*focused*/) const {}
  virtual void ClearContinuousAttack() {}
  virtual void Reset() {}

protected:
  explicit PlayerLoadout(PlayerTraits traits) : traits_(traits) {}

private:
  const PlayerTraits traits_;
};
