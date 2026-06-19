///
/// WideForm - WIDE weapon (spread shot) base and focus forms.
///

#pragma once

#include "weapon_form.h"

class WideForm : public WeaponForm {
 public:
  explicit WideForm(Player& p) : WeaponForm(p) {}
  void FireMain(uint8_t tier) override;
  void FireSub(uint8_t tier) override;
  void FireBomb() override;
  uint16_t BombDuration() const override;
};

class WideFocusForm : public WeaponForm {
 public:
  explicit WideFocusForm(Player& p) : WeaponForm(p) {}
  void FireMain(uint8_t tier) override;
  void FireSub(uint8_t tier) override;
};
