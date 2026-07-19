///
/// Bullet - Definitions and various things related to bullets
///

#pragma once

#include <cstdint>

#include "bullet_base.h"

////Bullet constants////
inline constexpr auto TAMA_MAX = (801 * 3);
inline constexpr auto TAMA_EVADE = 1;

inline constexpr auto TAMA1_POINT = 10000;
inline constexpr auto TAMA2_POINT = 15000;

inline constexpr auto TAMA_EVADE_RADIUS_SMALL = 24 * 64;
inline constexpr auto TAMA_EVADE_RADIUS_LARGE = 32 * 64;

inline constexpr auto TAMA_SMALL = 0x00;
inline constexpr auto TAMA_LARGE = 0x10;
inline constexpr auto TAMA_ANGLE = 0x20;
inline constexpr auto TAMA_EXTRA = 0x30;
inline constexpr auto TAMA_EXTRA2 = 0x40;
inline constexpr auto TAMA_REN = 0x04;
inline constexpr auto TAMA_ZSET = 0x08;
inline constexpr auto TAMASP_RND0 = 0x00;
inline constexpr auto TAMASP_RND1 = 0x40;
inline constexpr auto TAMASP_RND2 = 0x80;
inline constexpr auto TAMASP_RND3 = 0xc0;

inline constexpr int TAMA_HIT_S = 2.5 * 64;
inline constexpr int TAMA_HIT_M = 4.5 * 64;
inline constexpr int TAMA_HIT_L = 7.5 * 64;
inline constexpr int TAMA_HIT_XL = 10.5 * 64;

inline constexpr auto T_NORM = 0x00;
inline constexpr auto T_NORM_A = 0x01;
inline constexpr auto T_HOMING = 0x02;
inline constexpr auto T_HOMING_M = 0x03;
inline constexpr auto T_ROLL = 0x04;
inline constexpr auto T_ROLL_A = 0x05;
inline constexpr auto T_ROLL_R = 0x06;
inline constexpr auto T_GRAVITY = 0x07;
inline constexpr auto T_CHANGE = 0x08;
inline constexpr auto T_SBHOMING = 0x09;
inline constexpr auto T_SBHBOMB = 0x0a;

inline constexpr auto TOP_NONE = 0x00;
inline constexpr auto TOP_WAVE = 0x10;
inline constexpr auto TOP_ROLL = 0x20;
inline constexpr auto TOP_PURU = 0x30;
inline constexpr auto TOP_REFX = 0x40;
inline constexpr auto TOP_REFY = 0x50;
inline constexpr auto TOP_REFXY = 0x60;
inline constexpr auto TOP_DIV = 0x70;
inline constexpr auto TOP_BOMB = 0x80;

inline constexpr auto TC_WAY = 0x00;
inline constexpr auto TC_ALL = 0x01;
inline constexpr auto TC_RND = 0x02;
inline constexpr auto TC_WAYS = 0x04;
inline constexpr auto TC_ALLS = 0x05;
inline constexpr auto TC_RNDS = 0x06;
inline constexpr auto TC_WAYZ = 0x08;
inline constexpr auto TC_ALLZ = 0x09;
inline constexpr auto TC_RNDZ = 0x0a;
inline constexpr auto TC_WAYSZ = 0x0c;
inline constexpr auto TC_ALLSZ = 0x0d;
inline constexpr auto TC_RNDSZ = 0x0e;

inline constexpr auto TE_NONE = 0x00;
inline constexpr auto TE_ROLL1 = 0x10;
inline constexpr auto TE_ROLL2 = 0x20;
inline constexpr auto TE_WARN = 0x30;
inline constexpr auto TE_ROCK = 0x40;
inline constexpr auto TE_CIRCLE1 = 0x50;
inline constexpr auto TE_CIRCLE2 = 0x60;
inline constexpr auto TE_DELETE = 0xf0;

inline constexpr auto TF_NONE = 0x00;
inline constexpr auto TF_CLIP = 0x01;
inline constexpr auto TF_EVADE = 0x02;
inline constexpr auto TF_DELETE = 0x80;

int GetBulletHitRadius(uint8_t c);
int GetBulletEvadeRadius(uint8_t c);

////Bullet command struct (ECL parameter accumulator)////
struct BulletCommand {
  int x{}, y{};
  uint8_t d{}, dw{};
  uint8_t n{}, ns{};
  uint8_t v{};
  uint8_t c{};
  char a{};
  char vd{};
  uint8_t rep{};
  uint8_t cmd{}, type{}, option{};
};

////Pool capacities////
inline constexpr auto kBulletSmallMax = TAMA_MAX;
inline constexpr auto kBulletLargeMax = TAMA_MAX;

////Spawn parameter struct////
struct BulletSpawnInfo {
  int x{}, y{};
  int v_{};
  char a{};
  uint8_t d{}, dw{};
  uint8_t n{}, ns{};
  uint8_t c{};
  uint8_t vsp{};
  uint8_t option{};
  uint8_t type{};
  uint8_t rep{};
  int8_t vd{};
  uint8_t effect{};
  uint8_t cmd_type{};
  bool rapid{};
  bool zset{};
};

struct BulletManager;

////World context + side-effect result (passed to / returned from Bullet::Update)////
struct BulletUpdateInfo {
  int player_x, player_y;
  bool enemy_homing_valid;
  int enemy_homing_x, enemy_homing_y;

  struct UpdateResult {
    bool smoke_spawn = false;
    int smoke_x = 0, smoke_y = 0;
    bool division_requested = false;
    BulletCommand division_cmd;
    int division_cx = 0, division_cy = 0;
  };
};

////Bullet class////
struct Bullet : BulletBase<BulletSpawnInfo, BulletUpdateInfo> {
  using SpawnInfo = BulletSpawnInfo;
  using UpdateInfo = BulletUpdateInfo;

  friend struct BulletManager;
  friend class Player; // TODO: remove when player shots are refactored

  void Render() const override;
  bool IsDead() const override;
  void Kill() override;
  void Spawn(const BulletSpawnInfo &info) override;
  [[nodiscard]] HitResult CheckHit(int player_x, int player_y) const override;
  [[nodiscard]] UpdateResult Update(const UpdateInfo &info = {}) override;
  void RenderDebugHitbox(int mode) const override;

private:
  // ── Manager-internal ──────────────────────────────────────

  void MarkDead() { flag_ |= TF_DELETE; }
  [[nodiscard]] bool HasGrazed() const { return (flag_ & TF_EVADE) != 0; }
  void MarkGrazed() { flag_ |= TF_EVADE; }

  // ── Fields ────────────────────────────────────────────────
  int tx_{};
  int ty_{};
  int vx_{};
  int vy_{};
  int v0_{};
  char a_{};
  uint16_t d16_{};
  int8_t vd_{};
  uint8_t rep_{};
  uint8_t type_{};
  uint8_t option_{};
  uint8_t effect_{};
  mutable uint8_t flag_{};

  void MoveByType(const UpdateInfo &info, UpdateResult &result);
  void MoveByOption(UpdateResult &result);
  void MoveByEffect();
  void RevertToNormal();
  void Draw() const;
  static void DrawEffect(const Bullet *t);
};

//// Free function: build SpawnInfo from ECL command ////
[[nodiscard]] BulletSpawnInfo MakeBulletSpawnInfo(const BulletCommand &cmd,
                                                  int ox, int oy, bool scaling);
