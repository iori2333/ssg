///
/// LaserLoadout - Laser player traits and attack behavior.
///

#pragma once

#include "player_loadout.h"

class LaserLoadout final : public PlayerLoadout {
public:
  LaserLoadout();

  void FireMain(Player &player, int tier, bool focused) override;
  void UpdateBomb(Player &player, EnemyManager &enemies, EffectManager &effects,
                  int remaining) override;
  void Tick(Player &player) override;
  void ApplyContinuousAttack(const Player &player, EnemyManager &enemies,
                             bool focused) const override;
  void DrawBombForeground(const Player &player,
                          int remaining) const override;
  void DrawContinuousAttack(const Player &player, bool focused) const override;
  void ClearContinuousAttack() override;
  void Reset() override;

private:
  int beam_time_ = 0;
  int beam_group_ = 0;

  void StartBeam(const Player &player, int time);
  [[nodiscard]] uint8_t BombAngle(int remaining) const;
  static uint8_t LeftOrRightAngle(uint8_t angle, int index);
  static uint8_t RightAngle(uint8_t angle, int index);
  static uint8_t LeftAngle(uint8_t angle, int index);
};
