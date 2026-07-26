///
/// Enemy actor lifecycle, damage, and spawn control
///

#include <algorithm>
#include <cstddef>
#include <utility>

#include "boss_manager.h"
#include "enemy.h"
#include "enemy_system.h"

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/entity.h"
#include "core/game_manager.h"
#include "core/gian.h"
#include "core/level.h"
#include "effect/effect_manager.h"
#include "gfx/graphics_backend.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "stage/stage_session.h"
#include "util/cast.h"
#include "util/ut_math.h"

static void EnemyDrawBomb(int x, int y, uint32_t count);

void EnemySystem::ConsiderHomingTarget(const EnemyActor *e) {
  const int temp = (player_->Y() - e->y);

  if (temp < 0) {
    return;
  }

  if (temp < homing_distance_) {
    homing_distance_ = temp;
    homing_target_ = {.active = true, .x = e->x, .y = e->y};
  }
}

bool EnemySystem::LaserHitCheck(const EnemyActor *e, int ox, int oy,
                                uint8_t d) {
  const int chkw = (std::min(e->g_height, e->g_width) + (3 * 64));

  const int tx = (e->x - ox);
  const int ty = (e->y - oy);

  const int l = (cosl(d, tx) + sinl(d, ty));
  const int w = abs(-sinl(d, tx) + cosl(d, ty));

  return ((l > 0) && (w < chkw));
}

void EnemySystem::DrawActor(const EnemyActor &actor) const {
  constexpr auto sid = SURFACE_ID::ENEMY;

  // TODO: Remove once the structure itself uses WORLD_POINT.
  const WORLD_POINT center = {&actor.x, &actor.y};

  const auto &a = anime[actor.anm_ptn];
  const auto topleft = center.ToPixel(a.size); // Coordinate set

  // Drawing mode selection
  const auto &src = (a.mode == ANM_DEG)
                        ? a.ptn[static_cast<uint8_t>(actor.d - 64 + 8) >> 4]
                        : a.ptn[actor.anm_c];
  if (GrpSurface_Blit({topleft.x, topleft.y}, sid, src)) {
    if ((actor.anm_ptn != actor.anm_ptnEx) && (actor.IsDamaged != 0U)) {
      const auto &a = anime[actor.anm_ptnEx];
      const auto topleft = center.ToPixel(a.size); // Coordinate set
      GrpSurface_Blit({topleft.x, topleft.y}, sid, a.ptn[0]);
    }
  }
}

void EnemySystem::MoveRegular() {
  // ECL can spawn another regular enemy while this loop is running. Query the
  // pool size on every iteration so each actor keeps the legacy same-frame
  // update behavior.
  for (std::size_t i = 0; i < regular_enemies_.Size(); ++i) {
    auto *e = &regular_enemies_.Active(i);
    e->IsDamaged = 0;
    if ((e->flag & EF_BOMB) == 0) {
      // Normal enemy processing
      CheckInterrupts(e);
      Execute(e);

      // Branch by bullet fire mode
      if ((e->t_rep != 0U) && (e->hp != 0U)) {
        e->tama_c = (e->tama_c + 1) % (e->t_rep);
        if (e->tama_c == 0) {
          auto si = MakeBulletSpawnInfo(e->t_cmd, e->x, e->y, true, *game_);
          bullets_->SpawnBullet(si);
        }
      }

      // Cactus hit check
      if (HITCHK(e->x, player_->X(), e->g_width) &&
          HITCHK(e->y, player_->Y(), e->g_height) &&
          player_->IsInvincible() == 0) {
        // Might be interesting to damage the enemy around here?
        if ((e->flag & EF_HITSB) != 0) {
          player_->OnHit();
        }
      }

      // Out-of-bounds check
      if ((e->y < GY_MIN - (e->g_height)) || (e->y > GY_MAX + (e->g_height)) ||
          (e->x < GX_MIN - (e->g_width)) || (e->x > GX_MAX + (e->g_width))) {
        if ((e->flag & EF_CLIP) == 0) {
          if (e->LLaserRef != 0U) {
            bullets_->ControlLongLaser(
                e, ECL_ALL_LONG_LASERS,
                LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
          }
          e->flag = EF_DELETE;
        }
      }
    } else if (e->count >= (8 * ENEMY_BOMB_SPD) - 1) {
      e->flag = EF_DELETE;
    }

    // Homing preparation
    if ((bosses_.ActiveCount() == 0) && ((e->flag & EF_DAMAGE) != 0)) {
      ConsiderHomingTarget(e);
    }

    // Animation update
    UpdateAnimation(e);

    e->count++;
  }

  regular_enemies_.Compact(
      [](const EnemyActor &e) { return (e.flag & EF_DELETE); });
}

void EnemySystem::DrawRegular() {
  int x = 0;
  int y = 0;
  // HRESULT		ddrval;

  for (auto &actor : regular_enemies_) {
    auto *e = &actor;

    // Draw enemy (add clipping & width, height handling)
    x = (e->x >> 6);
    y = (e->y >> 6);
    if (e->flag == EF_BOMB) {
      EnemyDrawBomb(x, y, e->count);
      continue;
    }

    if ((e->flag & EF_DRAW) != 0) {
      DrawActor(*e);
    }
  }
}

// Clear small enemies
void EnemySystem::ClearRegular() {
  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if (e->flag == EF_BOMB) {
      continue;
    }

    if ((e->flag & EF_DRAW) != 0) {
      e->flag = EF_BOMB;
      e->hp = 0;
      e->count = 0;
      if (e->LLaserRef != 0U) {
        bullets_->ControlLongLaser(
            e, ECL_ALL_LONG_LASERS,
            LongLaserUpdateInfo{
                LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
      }
      Snd_SEPlay(SfxId::Bomb, e->x);
    } else {
      // Erasing non-drawing type enemies differs from other cases:
      // do not play explosion animation/sound
      e->flag = EF_DELETE;
      e->hp = 0;
      e->count = 0;
      if (e->LLaserRef != 0U) {
        bullets_->ControlLongLaser(
            e, ECL_ALL_LONG_LASERS,
            LongLaserUpdateInfo{
                LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
      }
      // Do not play explosion sound
    }
  }

  regular_enemies_.Compact(
      [](const EnemyActor &e) { return (e.flag & EF_DELETE); });
}

void EnemySystem::ResetRegular() { regular_enemies_.Init(); }

bool EnemySystem::ApplyDamage(EnemyActor &e, int damage) {
  e.IsDamaged = ((e.count) & 1);
  if (std::cmp_less_equal(e.hp, damage)) {
    Snd_SEPlay(SfxId::Bomb, e.x);
    if (e.LLaserRef != 0U) {
      bullets_->ControlLongLaser(
          &e, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    player_->PowerUp(static_cast<uint8_t>(e.hp)); // Power up
    e.hp = 0;
    e.count = 0;
    e.flag = EF_BOMB;
    player_->AddScore(e.score);
    if (e.item != 0U) {
      items_->Spawn(e.x, e.y, e.item);
    }
  } else {
    Snd_SEPlay(SfxId::Hit, e.x);
    player_->PowerUp(damage); // Power up here too
    e.hp -= damage;
  }
  return true;
}

bool EnemySystem::DamageAt(int x, int y, int damage) {
  if (bosses_.DamageAt(x, y, damage)) {
    return true;
  }

  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if (HITCHK(x, e->x, e->g_width) && HITCHK(y, e->y, e->g_height) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      return ApplyDamage(*e, damage);
    }
  }

  return false;
}

bool EnemySystem::DamageAt2(int x, int y, int damage) {
  auto ret_val = bosses_.DamageAt2(x, y, damage);

  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if (HITCHK(x, e->x, e->g_width) && (y > e->y) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ret_val = ApplyDamage(*e, damage);
    }
  }

  return ret_val;
}

// Diagonal laser hit detection
void EnemySystem::DamageAt3(int x, int y, uint8_t d) {
  // bool	ret_val = false;
  constexpr int damage = 8;

  bosses_.DamageAt3(x, y, d);

  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if (EnemySystem::LaserHitCheck(e, x, y, d) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ApplyDamage(*e, damage);
    }
  }
}

// Damage all enemies
void EnemySystem::DamageAll(int damage) {
  bosses_.DamageAll(damage);

  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if ((e->flag & EF_DAMAGE) != 0) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ApplyDamage(*e, damage);
      // return true;
    }
  }

  // return false;
}

void EnemySystem::InitializeActor(EnemyActor &actor, WORLD_POINT position,
                                  uint32_t script_id) {
  const auto entry = program_.Entry(script_id);
  if (!entry) {
    actor.flag = EF_DELETE;
    return;
  }

  actor.x = position.x;
  actor.y = position.y;

  actor.script = {};
  actor.script.position = *entry;
  actor.script.return_position = actor.script.position;

  actor.hp = 0xffffffff;
  actor.amp = 0;
  actor.anm_ptn = 0;
  actor.anm_ptnEx = 0; // Added: 2000/11/27 (animation while damaged)
  actor.anm_sp = 0;
  actor.anm_c = 0;
  actor.count = 0;
  actor.evscore = 0;
  actor.d = 64;
  actor.flag = EF_DAMAGE | EF_DRAW | EF_HITSB;

  actor.IsDamaged = 0;

  actor.tama_c = Cast::down<uint8_t>(rnd()); // & 0xff;
  actor.t_rep = 0; // Bullet fire interval (0: no auto-fire)
  actor.g_width = 0;
  actor.g_height = 0;

  actor.item = ITEM_SCORE;

  actor.script.loop_counter = 0;
  actor.script.wait_counter = 0;
  actor.v = 64;
  actor.vd = 0;
  actor.vx = cosl(actor.d, actor.v);
  actor.vy = sinl(actor.d, actor.v);

  actor.LLaserRef = 0;

  actor.t_cmd.c = 0;
  actor.t_cmd.cmd = TC_WAY;
  actor.t_cmd.d = 64;
  actor.t_cmd.n = 1;
  actor.t_cmd.option = TE_NONE;
  actor.t_cmd.type = T_NORM;
  actor.t_cmd.v = 3;
  actor.t_cmd.x = 0;
  actor.t_cmd.y = 0;

  actor.t_cmd.dw = 16;
  actor.t_cmd.ns = 1;
  actor.t_cmd.rep = 0;
  actor.t_cmd.vd = 0;

  actor.l_cmd.l2 = 0;
  actor.l_cmd.x = 0;
  actor.l_cmd.y = 0;
  actor.l_cmd.notr = 0xff;
}

// Force move between ECL blocks
void EnemySystem::JumpToScript(EnemyActor *e, uint32_t script_id) {
  const auto entry = program_.Entry(script_id);
  if (!entry) {
    e->flag = EF_DELETE;
    return;
  }
  e->script.position = *entry;
  e->script.return_position = e->script.position;

  e->t_rep = 0; // Bullet fire interval (0: no auto-fire)
  e->script.loop_counter = 0;
  e->script.wait_counter = 0;
}

EnemyActor *EnemySystem::SpawnRegular(WORLD_POINT position,
                                      uint32_t script_id) {
  auto *actor = regular_enemies_.Alloc();
  if (actor == nullptr) {
    return nullptr;
  }
  InitializeActor(*actor, position, script_id);
  return actor;
}

void EnemySystem::SpawnFromScene(int16_t x, int16_t y, uint8_t script_id) {
  WORLD_POINT position;
  position.x = x == X_RNDV ? GX_RND() : PixelToWorld(x);
  position.y = y == Y_RNDV ? GY_RND() : PixelToWorld(y);
  SpawnRegular(position, script_id);
}

void EnemySystem::UpdateAnimation(EnemyActor *e) {
  const auto *a = &anime[e->anm_ptn];

  switch (a->mode) {
  case ANM_NORM:
    if (e->anm_sp > 0 && (e->count % e->anm_sp == 0)) {
      e->anm_c = (e->anm_c + 1) % (a->n);
    } else if (e->anm_sp < 0 && (e->count % (-e->anm_sp) == 0)) {
      e->anm_c = (e->anm_c + a->n - 1) % (a->n);
    }
    break;

  // Reverse direction is not allowed...
  case ANM_STOP:
    if (e->anm_sp > 0 && (e->count % e->anm_sp == 0)) {
      if (e->anm_c < (a->n - 1)) {
        e->anm_c++;
      }
    }
    break;

  default:
    break;
  }
}

static void EnemyDrawBomb(int x, int y, uint32_t count) {
  PIXEL_LTRB src;

  src.top = 296;
  src.left = (count / ENEMY_BOMB_SPD) * 48;
  src.bottom = 296 + 48;
  src.right = src.left + 48;

  x -= 24;
  y -= 24;

  GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
}
