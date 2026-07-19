///
/// BulletManager - Centralized bullet system state
///

#pragma once

#include <cstdint>

#include "bullet.h"
#include "core/object_pool.h"

struct BulletManager {
  ObjectPool<Bullet, kBulletSmallMax> pool_small;
  ObjectPool<Bullet, kBulletLargeMax> pool_large;

  void Init();

  bool Spawn(const BulletSpawnInfo &si);
  bool SpawnLine(const BulletSpawnInfo &si);
  bool SpawnExtra01(const BulletSpawnInfo &si);

  void UpdateAll();
  void HitCheckAll();
  void RenderAll() const;
  void ClearAll();

  void Move() { UpdateAll(); }
  void Draw() { RenderAll(); }
  void Clear() { ClearAll(); }

  uint32_t ScoreToItems();
  void ToItems(uint8_t n);
  void RenderDebugHitboxes(int mode) const;

  // Gallery / debug helpers
  void PlaceDisplayBullet(int x, int y, uint8_t color);
  void RotateDisplayAngles();

  // ── Player shot compat (TODO: remove after player shot refactor) ──

  BulletCommand command; // deprecated

  [[nodiscard]] uint8_t Dir(uint16_t i) const;
  [[nodiscard]] int Speed(uint16_t i) const;
  [[nodiscard]] uint8_t Flag() const;
  void MoveByOption(Bullet *t);

  static void MoveByType(Bullet *t);
  static void MoveByEffect(Bullet *t);

private:
  template <typename Pool>
  void SpawnImpl(const BulletSpawnInfo &si, Pool &pool);

  template <typename Fn> void ApplySmall(Fn fn) {
    for (auto &b : pool_small) {
      fn(b);
    }
  }
  template <typename Fn> void ApplyLarge(Fn fn) {
    for (auto &b : pool_large) {
      fn(b);
    }
  }
};

extern BulletManager Bullets;

//// Bullet command inline helpers (set Bullets.command) ////

inline void TamaSetForm(uint8_t cmd, uint8_t option, uint8_t type, uint8_t c) {
  Bullets.command.cmd = cmd;
  Bullets.command.option = option;
  Bullets.command.type = type;
  Bullets.command.c = c;
}

inline void TamaSTDForm(uint8_t c) { TamaSetForm(TC_WAY, TOP_NONE, T_NORM, c); }

inline void TamaSetDeg(uint8_t d, uint8_t dw) {
  Bullets.command.d = d;
  Bullets.command.dw = dw;
}

inline void TamaSetNum(uint8_t n, uint8_t ns) {
  Bullets.command.n = n;
  Bullets.command.ns = ns;
}

inline void TamaSetSpd(uint8_t v, char a) {
  Bullets.command.v = v;
  Bullets.command.a = a;
}

inline void TamaSetXY(int x, int y) {
  Bullets.command.x = x;
  Bullets.command.y = y;
}
