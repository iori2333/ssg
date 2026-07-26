///
/// LaserLoadout - Laser player traits and attack behavior.
///

#pragma once

#include "player_loadout.h"

class LaserLoadout final : public PlayerLoadout {
public:
  LaserLoadout();

  void FireMain(Player &player, uint8_t tier, bool focused) override;
  void FireSub(Player &player, uint8_t tier, bool focused) override {}
  void UpdateBomb(Player &player, EnemyManager &enemies,
                  uint16_t remaining) override;
  void Tick(Player &player) override;
  void ApplyContinuousAttack(const Player &player, EnemyManager &enemies,
                             bool focused) const override;
  void DrawBombForeground(const Player &player,
                          uint16_t remaining) const override;
  void DrawContinuousAttack(const Player &player, bool focused) const override;
  void ClearContinuousAttack() override;
  void Reset() override;

private:
  uint16_t beam_time_ = 0;
  uint8_t beam_group_ = 0;

  void StartBeam(const Player &player, uint16_t time);
  [[nodiscard]] uint8_t BombAngle(uint16_t remaining) const;
  static uint8_t LeftOrRightAngle(uint8_t angle, int index);
  static uint8_t RightAngle(uint8_t angle, int index);
  static uint8_t LeftAngle(uint8_t angle, int index);
};
