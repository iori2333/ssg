///
/// LaserForm - LASER weapon (straight beam) base and focus forms.
///

#pragma once

#include "weapon_form.h"

class LaserForm : public WeaponForm {
public:
  explicit LaserForm(Player &p) : WeaponForm(p) {}
  void FireMain(uint8_t tier) override;
  void FireSub(uint8_t tier) override {}
  void FireBomb(EnemyManager &enemies) override;
  uint16_t BombDuration() const override;
  void OnFireTick() override;
  void OnCollisionTick(EnemyManager &enemies) override;
};

class LaserFocusForm : public WeaponForm {
public:
  explicit LaserFocusForm(Player &p) : WeaponForm(p) {}
  void FireMain(uint8_t tier) override;
  void FireSub(uint8_t tier) override {}
  void OnFireTick() override;
  void OnCollisionTick(EnemyManager &enemies) override;
};
