///
/// HomingForm - HOMING weapon (tracking sub-shots) base and focus forms.
///

#pragma once

#include "weapon_form.h"

class HomingForm : public WeaponForm {
public:
  explicit HomingForm(Player &p) : WeaponForm(p) {}
  void FireMain(uint8_t tier) override;
  void FireSub(uint8_t tier) override;
  void FireBomb(EnemyManager &enemies) override;
  uint16_t BombDuration() const override;
};

class HomingFocusForm : public WeaponForm {
public:
  explicit HomingFocusForm(Player &p) : WeaponForm(p) {}
  void FireMain(uint8_t tier) override;
  void FireSub(uint8_t tier) override;
};
