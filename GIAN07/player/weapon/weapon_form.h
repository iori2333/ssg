///
/// WeaponForm - Abstract base for player attack forms.
///
/// Each weapon (WIDE / HOMING / LASER) has a base form and a focus
/// (low-speed) form.  The Player owns one instance of each and
/// delegates per-frame firing to the active form.
///

#pragma once

#include <cstdint>

class Player;

class WeaponForm {
public:
  virtual ~WeaponForm() = default;

  // Main-shot fire, called on IsMainShotFrame frames.
  virtual void FireMain(uint8_t tier) = 0;

  // Sub-shot (option) fire, called on IsSubShotFrame frames.
  virtual void FireSub(uint8_t tier) = 0;

  // Bomb update, called every frame while bomb_time > 0.
  // Only base forms are dispatched for bombs; focus forms inherit
  // the empty default.
  virtual void FireBomb() {}

  // Bomb duration in frames when this form's bomb is activated.
  virtual uint16_t BombDuration() const { return 0; }

  // Per-frame tick (always called, regardless of toge_time).
  // Laser forms override to manage lay_time / lay_grp countdown.
  virtual void OnFireTick() {}

  // Per-frame collision check for weapon-specific continuous beams
  // (laser).  Called from MoveMaidShot after bullet hit detection.
  virtual void OnCollisionTick() {}

protected:
  explicit WeaponForm(Player &player) : player_(player) {}
  Player &player_;
};
