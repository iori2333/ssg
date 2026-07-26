///
/// WideLoadout - Wide player traits and attack behavior.
///

#pragma once

#include "player_loadout.h"

class WideLoadout final : public PlayerLoadout {
public:
  WideLoadout();

  void FireMain(Player &player, uint8_t tier, bool focused) override;
  void FireSub(Player &player, uint8_t tier, bool focused) override;
  void UpdateBomb(Player &player, EnemyManager &enemies,
                  uint16_t remaining) override;
  void DrawBombBackground(const Player &player,
                          uint16_t remaining) const override;
  void Reset() override { shot_phase_ = 0; }

private:
  uint8_t shot_phase_ = 0;

  void FireMainNormal(Player &player, uint8_t tier);
  static void FireMainFocused(Player &player, uint8_t tier);
  void FireSubNormal(Player &player, uint8_t tier);
  void FireSubFocused(Player &player, uint8_t tier);
};
