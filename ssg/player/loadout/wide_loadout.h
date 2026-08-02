///
/// WideLoadout - Wide player traits and attack behavior.
///

#pragma once

#include "player_loadout.h"

class WideLoadout final : public PlayerLoadout {
public:
  WideLoadout();

  void FireMain(Player &player, int tier, bool focused) override;
  void FireSub(Player &player, int tier, bool focused) override;
  void UpdateBomb(Player &player, EnemyManager &enemies, EffectManager &effects,
                  int remaining) override;
  void DrawBombBackground(const Player &player,
                          int remaining) const override;
  void Reset() override { shot_phase_ = 0; }

private:
  int shot_phase_ = 0;

  void FireMainNormal(Player &player, int tier);
  static void FireMainFocused(Player &player, int tier);
  void FireSubNormal(Player &player, int tier);
  void FireSubFocused(Player &player, int tier);
};
