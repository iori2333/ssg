///
/// EnemyManager - centralized enemy system state and operations
///

#pragma once

#include <array>
#include <cstdint>

#include "enemy.h"
#include "sys/buffer.h"

struct BulletManager;
struct ItemManager;
struct LaserManager;
struct GameManager;
class Player;

struct EnemyManager {
  // --- Enemy data ---
  std::array<EnemyData, ENEMY_MAX> entities; // Enemy[]
  std::array<uint16_t, ENEMY_MAX> indices;   // EnemyInd[]
  uint16_t count = 0;                        // EnemyNow

  // --- ECL/SCL data ---
  BYTE_BUFFER_BORROWED ecl_head;    // ECL_Head
  BYTE_BUFFER_BORROWED scl_head;    // SCL_Head
  const uint8_t *scl_now = nullptr; // SCL_Now

  // --- Animation ---
  ANIME_DATA anime[ANIME_MAX]; // Anime[]

  // --- Homing ---
  int homing_x = 0;    // HomingX
  int homing_y = 0;    // HomingY
  int homing_flag = 0; // HomingFlag

  // --- Special angle (used in ENEMY.cpp) ---
  uint8_t enemy_exdeg = 0;   // EnemyEXDEG
  uint8_t enemy_exdeg_d = 0; // EnemyEXDEG_D

  BulletManager *bullets_ = nullptr;
  LaserManager *lasers_ = nullptr;
  ItemManager *items_ = nullptr;
  GameManager *game_ = nullptr;
  Player *player_ = nullptr;

  // --- DI ---
  void Bind(BulletManager &bm) { bullets_ = &bm; }
  void Bind(LaserManager &lm) { lasers_ = &lm; }
  void Bind(ItemManager &im) { items_ = &im; }
  void Bind(GameManager &gm) { game_ = &gm; }
  void Bind(Player &p) { player_ = &p; }

  // === Methods ===

  // Homing
  void UpdateHoming(const EnemyData *e);

  // Diagonal laser hit check (utility)
  static bool LaserHITCHK(const EnemyData *e, int ox, int oy, uint8_t d);

  // Enemy movement, drawing and management
  void Move();
  void Draw();
  void Clear();
  void InitIndices();

  // Damage
  bool ApplyDamage(EnemyData &e, int damage);
  bool DamageAt(int x, int y, int damage);
  bool DamageAt2(int x, int y, int damage);
  void DamageAt3(int x, int y, uint8_t d);
  void DamageAll(int damage);

  // Enemy data initialization
  void InitDataX64(EnemyData *e, int x, int y, uint32_t EclID);
  void InitDataSTD(EnemyData *e, short x, short y, uint32_t EclID);
  void LongJump(EnemyData *e, uint32_t EclID);

  // Animation
  void UpdateAnimation(EnemyData *e);

  // ECL
  void Execute(EnemyData *e);
  static void CheckInterrupts(EnemyData *e);
  static void InitInterrupts(EnemyData *e);
};

extern EnemyManager Enemies;
