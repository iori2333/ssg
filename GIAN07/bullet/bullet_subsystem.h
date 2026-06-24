///
/// BulletSubsystem - owns the enemy bullet pool, the player shot pool
/// (formerly `maid_tama_*` in Player), and all spawn / move / draw / clear
/// logic for both.
///

#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "bullet_data.h"
#include "world_refs.h"

namespace bullets {

class BulletSubsystem {
public:
  explicit BulletSubsystem(world::Refs w);

  void Reset(); ///< Initialize pools (enemy + player).
  void Move();  ///< Per-frame enemy bullet update.
  void Draw();  ///< Enemy bullet draw.
  void Clear(); ///< Mark all enemy bullets for delete effect.

  uint32_t ScoreToItems();
  void ToItems(uint8_t n);

  // --- Enemy spawn API --------------------------------------------
  void Spawn(const BulletCommand &cmd);   ///< Apply difficulty + rank.
  void SpawnEX(const BulletCommand &cmd); ///< No difficulty scaling.
  void SpawnLine(const BulletCommand &cmd);
  void SpawnExtra01(const BulletCommand &cmd);

  // --- Player shot API (replaces Player::SpawnShot_ / maid shot) -
  void SpawnPlayer(const BulletCommand &cmd);
  void MovePlayer();
  void DrawPlayer();
  void ResetPlayerIndices();

  // --- Static movement helpers (shared with ex. player_shot call sites) -
  static void MoveByType(Bullet *t, world::Refs w);
  static void MoveByEffect(Bullet *t);
  void MoveByOption(Bullet *t);

  // --- Read-only views (debug overlay / external queries) -------
  // `EnemySmallActive()` / `EnemyLargeActive()` / `PlayerActive()` return
  // the full backing arrays — callers must filter out inactive slots by
  // their own criteria (flag, count).  Use the index spans to walk only
  // live slots.
  std::span<const Bullet> AllEnemySmall() const { return enemy_small_bullets_; }
  std::span<const Bullet> AllEnemyLarge() const { return enemy_large_bullets_; }
  std::span<const Bullet> AllPlayer() const { return player_bullets_; }
  std::span<Bullet> AllEnemySmallUnsafe() { return enemy_small_bullets_; }
  std::span<Bullet> AllEnemyLargeUnsafe() { return enemy_large_bullets_; }
  std::span<uint16_t> EnemySmallIndicesUnsafe() {
    return {enemy_small_idx_.data(), enemy_small_now_};
  }
  std::span<uint16_t> EnemyLargeIndicesUnsafe() {
    return {enemy_large_idx_.data(), enemy_large_now_};
  }
  std::span<const uint16_t> EnemySmallIndices() const {
    return {enemy_small_idx_.data(), enemy_small_now_};
  }
  std::span<const uint16_t> EnemyLargeIndices() const {
    return {enemy_large_idx_.data(), enemy_large_now_};
  }
  std::span<const uint16_t> PlayerIndices() const {
    return {player_idx_.data(), player_now_};
  }
  uint16_t EnemySmallNow() const { return enemy_small_now_; }
  uint16_t EnemyLargeNow() const { return enemy_large_now_; }
  uint16_t PlayerNow() const { return player_now_; }
  void SetEnemySmallNow(uint16_t n) { enemy_small_now_ = n; }
  void SetEnemyLargeNow(uint16_t n) { enemy_large_now_ = n; }
  // Rebuild indices as identity (used by bullet-gallery debug mode).
  void ResetEnemyIndices();

  // --- Capacity info ---
  static constexpr size_t kEnemySmallMax = 400;
  static constexpr size_t kEnemyLargeMax = (TAMA_MAX - 400);
  static constexpr size_t kPlayerMax = MAIDTAMA_MAX;

private:
  world::Refs world_;

  // Enemy bullet storage.
  std::array<Bullet, kEnemySmallMax> enemy_small_bullets_{};
  std::array<Bullet, kEnemyLargeMax> enemy_large_bullets_{};
  std::array<uint16_t, kEnemySmallMax> enemy_small_idx_{};
  std::array<uint16_t, kEnemyLargeMax> enemy_large_idx_{};
  uint16_t enemy_small_now_ = 0;
  uint16_t enemy_large_now_ = 0;

  // Player shot storage.
  std::array<Bullet, kPlayerMax> player_bullets_{};
  std::array<uint16_t, kPlayerMax> player_idx_{};
  uint16_t player_now_ = 0;

  // Cached spawn speed (set inside Spawn() family).
  int speed_ = 0;

  // --- private spawn helpers ---
  void TamaSetMain(const BulletCommand &cmd);
  // Allocates one enemy bullet slot in the pool selected by `c`; returns
  // nullptr if that pool is full.
  Bullet *AllocEnemy(uint8_t c);
  void SetEasy(BulletCommand &cmd) const;
  void SetHard(BulletCommand &cmd) const;
  void SetLunatic(BulletCommand &cmd) const;
  uint8_t Dir(uint16_t i, const BulletCommand &cmd) const;
  int NewSpeed(uint16_t i, const BulletCommand &cmd) const;
  int LineCmdNewSpeed(uint16_t i, const BulletCommand &cmd) const;
  int SpeedEx(uint8_t d, const BulletCommand &cmd) const;
  int SpeedFromCmd(uint16_t i, const BulletCommand &cmd) const;
  uint8_t FlagForCmd(const BulletCommand &cmd) const;
};

} // namespace bullets