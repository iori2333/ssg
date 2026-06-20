///
/// Boss.cpp   Boss processing (including mid-bosses)
///

#include "boss.h"

#include <algorithm>
#include <format>

#include "bomb_efc.h" // Explosion effect processing
#include "boss_manager.h"
#include "enemy_ex_ctrl.h"
#include "font_uty.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "geometry.h"
#include "gian.h"
#include "loader.h"
#include "platform/graphics_backend.h"
#include <utility>

///// [ Constants ] /////

// BOSS_MAX, BOSSHPG_HEIGHT moved to boss.h

// Boss states
static constexpr auto BEXST_NORM = 0x00; // Normal ECL operation
static constexpr auto BEXST_DEAD =
    0x01; // Dead <- probably not in use (2000/10/31)
static constexpr auto BEXST_WING01 = 0x02; // Butterfly wings
static constexpr auto BEXST_WING02 = 0x03; // Angel wings
static constexpr auto BEXST_SHILD1 = 0x04; // Shield 1
static constexpr auto BEXST_SHILD2 = 0x05; // Shield 2

// HP gauge
static constexpr auto BOSSHPG_WIDTH = 256; // HP gauge width
// BOSSHPG_HEIGHT moved to boss.h
static constexpr auto BOSSHPG_START_X = X_MAX; // Initial X of HP gauge
static constexpr auto BOSSHPG_END_X = 260;     // Final X of HP gauge

static constexpr auto BHPG_DEAD = 0x00;  // HP gauge is not in use
static constexpr auto BHPG_OPEN1 = 0x01; // Open HP gauge (during first effect)
static constexpr auto BHPG_OPEN2 = 0x02; // Open HP gauge (HP increasing)
static constexpr auto BHPG_NORM = 0x03;  // HP gauge is ready
static constexpr auto BHPG_CLOSE = 0x04; // Close HP gauge
static constexpr auto BHPG_OPEN3 = 0x05; // Update HP gauge

///// [Structs] /////

// BOSSHPG_INFO moved to boss.h

///// [ Variables ] /////

// bosses[], count, hpg moved to BossManager in boss_manager.cpp

// Private functions
static void HPG_Open(uint32_t max);    // Open the boss HP gauge
static void HPG_Move(uint32_t now);    // Increase/decrease the boss HP gauge
static void HPG_Close();               // Close the boss HP gauge
static void HPG_Update(uint32_t next); // Raise the boss HP gauge

static int PutBoss(int x, int y, uint32_t id); // Set a boss
static void STDMove(BossData *b);              // Normal ECL-compatible movement

// Initialize boss data array (used on interrupt, stage clear)
void BossManager::Init() {
  for (auto &it : bosses) {
    it.IsUsed = false; // Basically just zero this

    // Initialize special variables... //
    it.ExMove = BossManager::STDMove; // Special movement function
    it.ExCount = 0;
    it.ExState = BEXST_NORM; // Special state variable
    it.Hit = nullptr;        // Special hit detection
  }

  // Boss is dead, so of course don't display HP gauge
  hpg.State = BHPG_DEAD;

  // Set boss count to 0
  count = 0;

  // Initialize snake management (somewhat mysterious)
  SnakyInit();

  // Also initialize bit management
  BitInit();
}

// Set a boss
void BossManager::Set(int x, int y, uint32_t BossID) {
  int n = 0;
  uint32_t HP_Sum = 0;

  // Convert to x64 coordinates and set the boss
  n = PutBoss(x << 6, y << 6, BossID);

  if (n == BOSS_MAX) {
    return; // If we get here, it's a bug
  }

  bosses[n].ExCount = 0;
  bosses[n].ExMove = BossManager::STDMove;
  bosses[n].ExState = BEXST_NORM;
  bosses[n].IsUsed = true;

  Enemies.Execute(&(bosses[n].Edat));
  // ObjectLockOn(&(bosses[n].Edat.x),&(bosses[n].Edat.y),bosses[n].Edat.g_width,bosses[n].Edat.g_height);

  for (const auto &it : bosses) {
    if (it.IsUsed) {
      HP_Sum += it.Edat.hp;
    }
  }

  HPG_Open(HP_Sum);
  count++;
}

// Set a boss (for ECL)
void BossManager::SetEx(int x, int y, uint32_t BossID) {
  int n = 0;
  uint32_t HP_Sum = 0;

  // Convert to x64 coordinates and set the boss
  n = PutBoss(x << 6, y << 6, BossID);

  if (n == BOSS_MAX) {
    return; // If we get here, it's a bug
  }

  bosses[n].ExCount = 0;
  bosses[n].ExMove = BossManager::STDMove;
  bosses[n].ExState = BEXST_NORM;
  bosses[n].IsUsed = true;

  Enemies.Execute(&(bosses[n].Edat));
  // ObjectLockOn(&(bosses[n].Edat.x),&(bosses[n].Edat.y),bosses[n].Edat.g_width,bosses[n].Edat.g_height);

  for (const auto &it : bosses) {
    if (it.IsUsed) {
      HP_Sum += it.Edat.hp;
    }
  }

  HPG_Update(HP_Sum);
  count++;
}

// Move the boss
void BossManager::Move() {
  uint32_t HP_Sum = 0;
  EnemyData *e = nullptr;

  Enemies.homing_flag = HOMING_DUMMY;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);
      e->IsDamaged = 0;
      b->ExMove(b);

      // Cactus hit check //
      if (HITCHK(e->x, Players.X(), e->g_width) &&
          HITCHK(e->y, Players.Y(), e->g_height) &&
          Players.IsInvincible() == 0) {
        // Might be interesting to deal damage to the enemy around here? //
        if ((e->flag & EF_HITSB) != 0) {
          Players.OnHit();
        }
      }

      // Prepare homing //
      if ((e->flag & EF_DAMAGE) != 0) {
        Enemies.UpdateHoming(e);
      }

      // Sum total HP //
      HP_Sum += b->Edat.hp;

      // Run animation //
      Enemies.UpdateAnimation(e);

      e->count++;
    }
  }

  hpg.PhaseThresholdHp = -1;
  hpg.TimerMax = -1;
  hpg.TimerNow = 0;
  for (const auto &it : bosses) {
    if (it.IsUsed) {
      const auto &ve = it.Edat;
      if (hpg.PhaseThresholdHp < 0 && ve.Vect[ECLVECT_HP].vect != 0) {
        hpg.PhaseThresholdHp = static_cast<int32_t>(ve.Vect[ECLVECT_HP].value);
      }
      if (hpg.TimerMax < 0 && ve.Vect[ECLVECT_TIMER].vect != 0) {
        hpg.TimerMax = static_cast<int32_t>(ve.Vect[ECLVECT_TIMER].value);
        hpg.TimerNow = static_cast<int32_t>(ve.IntTimer);
      }
    }
  }

  SnakyMove();
  BitMove();
  HPG_Move(HP_Sum);
}

// Draw the boss
void BossManager::Draw() {
  constexpr auto sid = SURFACE_ID::ENEMY;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int t = 0;
  EnemyData *e = nullptr;
  PIXEL_LTRB wing;

  BitLineDraw();

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);

      x = (e->x >> 6);
      y = (e->y >> 6);

      // Spirit state //
      if (b->ExState == BEXST_SHILD2 && (Players.IsBombActive() != 0U) &&
          ((e->flag & EF_DRAW) != 0)) {
        wing = PIXEL_LTWH{
            (160 + ((Cast::sign<int32_t>(e->count / 2) % 4) * 40)), 80, 40, 40};

        // pbg quirk: Blitted without clipping?! I'd consider this a
        // bug if it wasn't explicitly commented as such. Fine then...
        GrpBackend_SetClip(GRP_RES_RECT);

        // No clipping
        GrpSurface_Blit({(x - 20), (y - 20)}, sid, wing);

        GrpBackend_SetClip({X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)});
        continue;
      }

      // Barrier state //
      if (b->ExState == BEXST_SHILD1 && (Players.IsBombActive() != 0U) &&
          ((e->flag & EF_DRAW) != 0)) {
        GrpGeom->Lock();
        for (uint8_t j = 0; j <= 5; j++) {
          GrpGeom->SetColor({(5U - j), (5U - j), 5U});
          GeomCircle({x, y}, (sinl((e->count * 4), (30 + (j * 4))) + 80));
        }
        GrpGeom->Unlock();
      }

      switch (b->ExState) {
      case BEXST_WING01:
        t = (b->ExCount - 64 - 8) << 2;
        t = std::max(t, 0);
        w = 64;
        h = 92;
        wing = {0, 176, 128, 360};
        GrpSurface_Blit({(x - w - t), (y - h)}, sid, wing);
        wing = {128, 176, 256, 360};
        GrpSurface_Blit({(x - w + t), (y - h)}, sid, wing);
        break;

      case BEXST_WING02:
        w = 44;
        h = 52;
        wing = {552, 0, 640, 104};
        GrpSurface_Blit({(x - w - 50), (y - h)}, sid, wing);
        wing = {552, 104, 640, 208};
        GrpSurface_Blit({(x - w + 50), (y - h)}, sid, wing);
        break;
      }

      if ((e->flag & EF_DRAW) != 0) {
        e->Draw();
      }
    }
  }
}

// Boss enemy bullet clear preprocessing function
void BossManager::ClearCmd() { BitDelete(); }

// Open the boss HP gauge
void BossManager::HPG_Open(uint32_t max) {
  int i = 0;

  hpg.Max = max;    // Max value
  hpg.Now = 0;      // Will increase during the first effect
  hpg.Next = max;   // Next HP value
  hpg.Update = max; // Update value

  hpg.State = BHPG_OPEN1;
  hpg.Count = 0;

  // Specify initial X for display (uses random but...)
  for (i = 0; i < BOSSHPG_HEIGHT; i++) {
    hpg.XTemp[i] = BOSSHPG_START_X + (i * 20);
  }
}

// Raise the boss HP gauge
void BossManager::HPG_Update(uint32_t next) {
  //	hpg.Max  = max;		// Max value
  //	hpg.Now  = 0;		// Will increase during the first effect
  hpg.Update = next; // Next HP value

  hpg.State = BHPG_OPEN3;
  //	hpg.Count = 0;
}

// Increase/decrease the boss HP gauge
void BossManager::HPG_Move(uint32_t now) {
  int i = 0;
  int ChkCount = 0;

  hpg.Next = now;

  switch (hpg.State) {
  case BHPG_OPEN1: {
    for (auto &it : hpg.XTemp) {
      it -= 6;
      if (it <= BOSSHPG_END_X) {
        it = BOSSHPG_END_X;
        ChkCount++;
      }
    }

    if (ChkCount == BOSSHPG_HEIGHT) {
      hpg.State = BHPG_OPEN2;
    }
  } break;

  case BHPG_OPEN2:
    hpg.Now += ((hpg.Max >> 7) + 1);
    if (hpg.Now >= hpg.Max) {
      hpg.Now = hpg.Max;
      hpg.State = BHPG_NORM;
    }
    break;

  case BHPG_OPEN3:
    hpg.Now += ((hpg.Max >> 7) + 1);
    if (hpg.Now >= hpg.Update) {
      hpg.Now = hpg.Update;
      hpg.State = BHPG_NORM;
    }
    break;

  case BHPG_NORM:
    if (hpg.Now > hpg.Next) {
      // temp = max(hpg.Max>>10,1);
      // temp = max((30*8*3)/max(hpg.Max,1),3);
      const auto temp =
          (std::max)(((std::max)(hpg.Max, 1U) / (30 * 8 * 4)), 3U);
      if (hpg.Now - hpg.Next > temp) {
        hpg.Now -= temp;
      } else {
        hpg.Now = hpg.Next;
      }
    }
    if (hpg.Now == 0) {
      HPG_Close();
    }
    break;

  case BHPG_CLOSE:
    hpg.XTemp[BOSSHPG_HEIGHT - 1] += 6;
    for (i = BOSSHPG_HEIGHT - 2; i >= 0; i--) {
      hpg.XTemp[i] = std::max(hpg.XTemp[i], hpg.XTemp[i + 1] - 20);
    }
    if (hpg.XTemp[0] >= BOSSHPG_START_X) {
      HPG_Close();
    }
    break;

  case BHPG_DEAD:
    // Of course do nothing
    return;
  }

  hpg.Count++;
}

// Close the boss HP gauge
void BossManager::HPG_Close() {
  // To be changed later
  hpg.State = BHPG_CLOSE;
}

// Draw the boss HP gauge
void BossManager::DrawHPG() {
  PIXEL_LTRB src;
  int i = 0;

  switch (hpg.State) {
  case BHPG_OPEN1:
  case BHPG_CLOSE:
    // Draw effect frame //
    for (i = 0; i < BOSSHPG_HEIGHT; i++) {
      src = {0, (104 + i), BOSSHPG_WIDTH, (104 + i + 1)};
      GrpSurface_Blit({hpg.XTemp[i], (16 + i)}, SURFACE_ID::SYSTEM, src);
    }
    break;

  case BHPG_OPEN2:
  case BHPG_NORM:
  case BHPG_OPEN3: {
    // Draw HP gauge //
    constexpr WINDOW_COORD left = (BOSSHPG_END_X + 3);
    constexpr WINDOW_COORD top = (16 + 3);
    constexpr WINDOW_COORD bottom = (top + 11);
    const auto x1 = (left + ((hpg.Next * 30 * 8) / hpg.Max));
    const auto x2 = (left + ((hpg.Now * 30 * 8) / hpg.Max));
    constexpr uint8_t alpha = (128 + 64);
    constexpr RGB216 col = {0, 1, 5};

    GrpGeom->Lock();
    GrpGeom->SetAlphaNorm(alpha);
    if (auto *gp = GrpGeom_Poly()) {
      VERTEX_XY Src[4] = {
          {0, top},
          {left, top},
          {left, bottom},
          {0, bottom},
      };
      if (x1 < x2) {
        Src[0].x = Src[3].x = x1;
        GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
        // gp->DrawBoxA(left, top, x1, bottom);
        gp->SetColor({5, 0, 0});
        gp->DrawBoxA(x1, top, x2, bottom);
      } else {
        Src[0].x = Src[3].x = x2;
        GeomGrdRectA(*gp, Src, col.ToRGB().WithAlpha(alpha));
        // gp->DrawBoxA(left, top, x2, bottom);
      }
    } else if (auto *gf = GrpGeom_FB()) {
      constexpr auto line_top = (top + 5);
      constexpr auto line_bottom = (bottom - 4);
      gf->SetColor(col);
      if (x1 < x2) {
        gf->DrawBoxA(left, top, x2, line_top);
        gf->DrawBoxA(left, line_bottom, x2, bottom);
        gf->SetColor({5, 5, 5});
        gf->DrawBoxA(left, line_top, x1, line_bottom);
        gf->SetColor({5, 0, 0});
        gf->DrawBoxA(x1, top, x2, bottom);
      } else {
        gf->DrawBoxA(left, top, x2, line_top);
        gf->DrawBoxA(left, line_bottom, x2, bottom);
        gf->SetColor({5, 5, 5});
        gf->DrawBoxA(left, line_top, x2, line_bottom);
      }
    }

    GrpGeom->Unlock();

    // Draw frame //
    src = {0, 104, BOSSHPG_WIDTH, 128};
    GrpSurface_Blit({BOSSHPG_END_X, 16}, SURFACE_ID::SYSTEM, src);

    if (hpg.PhaseThresholdHp > 0 && hpg.Max > 0) {
      const auto sep_x =
          left +
          static_cast<int32_t>(
              (static_cast<uint64_t>(hpg.PhaseThresholdHp) * 30 * 8) / hpg.Max);
      if (sep_x > left && sep_x < (left + 30 * 8)) {
        GrpGeom->Lock();
        GrpGeom->SetAlphaNorm(224);
        GrpGeom->SetColor({5, 5, 5});
        GrpGeom->DrawBoxA(sep_x, top, (sep_x + 3), bottom);
        GrpGeom->Unlock();
      }
    }

    if (hpg.TimerMax > 0) {
      const int remain = std::min((hpg.TimerMax - hpg.TimerNow) / 60, 99);
      if (remain >= 0) {
        if (remain <= 10 && remain != hpg.PrevTimerSeconds) {
          Snd_SEPlay(SOUND_ID_SBLASER);
        }
        hpg.PrevTimerSeconds = remain;
        if (remain < 10) {
          GrpSurface_SetColorMod(SURFACE_ID::SYSTEM, 255, 64, 64);
        }
        GrpPut16(476, 0, std::format("{:>2}", remain).c_str());
        if (remain < 10) {
          GrpSurface_SetColorMod(SURFACE_ID::SYSTEM, 255, 255, 255);
        }
      }
    }
  } break;

  case BHPG_DEAD:
    // Of course do nothing
    break;
  }
}

// Set HP of all currently active bosses to 0
void BossManager::KillAll() {
  // Uses the same functions as damaging/destroying for fragment emission, etc.
  // // But naturally, score & experience? are not obtainable // Don't forget to
  // close lasers too!!                                       //

  EnemyData *e = nullptr;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);
      SnakyDelete(b);
      Effects.SpawnFragment(e->x, e->y, FRG_FATCIRCLE);
      Effects.SpawnBombEffect(e->x, e->y, EXBOMB_STD);
      Snd_SEPlay(SOUND_ID_BOSSBOMB, e->x);
      if (e->LLaserRef != 0U) {
        Lasers.ForceCloseLong(e); // Force close laser
      }
      e->hp = 0;
      e->count = 0;
      e->flag = EF_BOMB;
      b->IsUsed = false;
    }
  }

  count = 0;
}

bool BossManager::ApplyDamage(BossData &b, EnemyData &e, int damage) {
  e.IsDamaged = ((e.count) & 1);
  if (std::cmp_less_equal(
          e.hp, damage)) { // Boss death processing (to be changed later!!)
    SnakyDelete(&b);
    BitDelete();
    Enemies.Clear();
    Effects.SpawnFragment(e.x, e.y, FRG_FATCIRCLE);
    Effects.SpawnBombEffect(e.x, e.y, EXBOMB_STD);
    Scroller.Command(SCMD_QUAKE);
    Snd_SEPlay(SOUND_ID_BOSSBOMB, e.x);
    if (e.LLaserRef != 0U) {
      Lasers.ForceCloseLong(&e); // Force close laser
    }
    Players.PowerUp(Cast::down<uint8_t>(e.hp));
    e.hp = 0;
    e.count = 0;
    e.flag = EF_BOMB;

    // If it was the last one //
    if (count == 1) {
      const auto temp = Bullets.ScoreToItems(); // Bullet -> score effect
      // sprintf(buf, "%3d Evade  %5dPts", Players.GrazeCount(),
      // Players.evadesc);
      Effects.SpawnStringEffect(
          180, 60, std::format("  Bonus    {:7}Pts", temp).c_str());
      Players.AddScore(temp);
    }

    if (e.item != 0U) {
      Items.Spawn(e.x, e.y, e.item);
    }
    Players.AddScore(e.score);
    Lasers.Clear();
    b.IsUsed = false;
    count--; // Uses boss reference count?
  } else {
    Snd_SEPlay(SOUND_ID_HIT, e.x);
    Players.PowerUp(damage);
    e.hp -= damage;
  }
  return true;
}

// Deal damage to boss
bool BossManager::DamageAt(int x, int y, int damage) {
  int i = 0;
  EnemyData *e = nullptr;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0) {
    return false;
  }

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.IsBombActive() != 0U) {
        continue;
      }
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (HITCHK(x, e->x, e->g_width) && HITCHK(y, e->y, e->g_height) &&
          ((e->flag & EF_DAMAGE) != 0)) {
        if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
          {
            continue;
          }
        }
        return ApplyDamage(*b, *e, damage);
      }
    }
  }
  return false;
}

// Deal damage to boss (y-axis upward infinite ver.)
bool BossManager::DamageAt2(int x, int y, int damage) {
  int i = 0;
  EnemyData *e = nullptr;
  bool ret_val = false;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0) {
    return false;
  }

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.IsBombActive() != 0U) {
        continue;
      }
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (HITCHK(x, e->x, e->g_width) && (y > e->y) &&
          ((e->flag & EF_DAMAGE) != 0)) {
        if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
          {
            continue;
          }
        }
        ret_val = ApplyDamage(*b, *e, damage);
      }
    }
  }
  return ret_val;
}

// Deal damage to boss (diagonal laser)
void BossManager::DamageAt3(int x, int y, uint8_t d) {
  int i = 0;
  EnemyData *e = nullptr;
  // BOOL			ret_val = FALSE;
  int damage = 2;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0) {
    return;
  }

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.IsBombActive() != 0U) {
        continue;
      }
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if (EnemyManager::LaserHITCHK(e, x, y, d) &&
          ((e->flag & EF_DAMAGE) != 0)) {
        if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
          {
            continue;
          }
        }
        ApplyDamage(*b, *e, damage);
      }
    }
  }
}

// Deal damage to boss (all enemies)
void BossManager::DamageAll(int damage) {
  int i = 0;
  EnemyData *e = nullptr;

  i = (BitGetNum() >> 1);
  damage -= i;
  if (damage <= 0) {
    return;
  }

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->ExState == BEXST_SHILD1 || b->ExState == BEXST_SHILD2) {
      if (Players.IsBombActive() != 0U) {
        continue;
      }
    }

    if (b->IsUsed) {
      e = &(b->Edat);
      if ((e->flag & EF_DAMAGE) != 0) {
        if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
          {
            continue;
          }
        }
        ApplyDamage(*b, *e, damage);

        // return TRUE;
      }
    }
  }
  //	return FALSE;
}

int BossManager::PutBoss(int x, int y, uint32_t id) {
  // if(EnemyNow+1>=ENEMY_MAX) return;
  // e = Enemy+ (*(EnemyInd+EnemyNow));
  // EnemyNow++;
  // n      = 4 + (((BYTE)p[4])<<2);
  // e->x   = (*(short *)(&p[0]));	//((int)(*(short *)(&p[0])))*64;
  // e->y   = (*(short *)(&p[2]));	//((int)(*(short *)(&p[2])))*64;

  auto it =
      std::ranges::find_if(bosses, [](const auto &it) { return !it.IsUsed; });

  // First, it shouldn't happen but... //
  if (it == std::end(bosses)) {
    return BOSS_MAX;
  }

  auto *e = &it->Edat;

  const uint32_t addr = (4 + (id << 2)); // A bit risky...
  Enemies.InitDataX64(e, x, y, addr);
  e->item = 0;

  return std::distance(std::begin(bosses), it);
}

// Normal ECL-compatible movement
void BossManager::STDMove(BossData *b) {
  EnemyData *e = &(b->Edat);

  // Normal enemy processing //
  EnemyManager::CheckInterrupts(e);
  Enemies.Execute(e);

  // Branch by bullet fire mode //
  if (e->t_rep != 0U) {
    e->tama_c = (e->tama_c + 1) % (e->t_rep);
    if (e->tama_c == 0) {
      Bullets.command = e->t_cmd;
      Bullets.command.x += e->x;
      Bullets.command.y += e->y;
      Bullets.Spawn();
    }
  }

  switch (b->ExState) {
  case BEXST_WING01:
    if (b->ExCount < 64 + 16 + 8) {
      b->ExCount++;
    }
    break;
  }
}

// Get the sum of all boss HP
uint32_t BossManager::GetHPSum() {
  uint32_t HP_Sum = 0;
  EnemyData *e = nullptr;

  for (auto &it : bosses) {
    auto *b = &it;
    if (b->IsUsed) {
      e = &(b->Edat);
      HP_Sum += e->hp;
    }
  }

  return HP_Sum;
}

// Boss interrupt processing
void BossManager::Interrupt(EnemyData *e, uint8_t IntID) {
  auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  // Branch by interrupt number //
  switch (IntID) {
  case ECLINT_SNAKEON:
    SnakySet(&*b, 30, 11);
    break;

  case ECLINT_LBWING01: // Also draw butterfly wings
    b->ExState = BEXST_WING01;
    b->ExCount = 0;
    break;

  case ECLINT_LBWING02: // Also draw bird wings
    b->ExState = BEXST_WING02;
    b->ExCount = 0;
    break;

  case ECLINT_BITON5:
    BitSet(&*b, 5, 3);
    break;

  case ECLINT_BITON6:
    BitSet(&*b, 6, 3);
    break;

  case ECLINT_SHILD1:
    b->ExState = BEXST_SHILD1;
    break;

  case ECLINT_SHILD2:
    b->ExState = BEXST_SHILD2;
    break;
  }
}

// Bit attack address specification
void BossManager::BitAttack(EnemyData *e, uint32_t AtkID) {
  const auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  BitSelectAttack(AtkID);
}

// Set laser command to bit
void BossManager::BitLaser(EnemyData *e, uint8_t cmd) {
  const auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  BitLaserCommand(cmd);
}

// Send bit command
void BossManager::BitCommand(EnemyData *e, uint8_t Cmd, int Param) {
  const auto b = std::ranges::find_if(
      bosses, [e](const auto &b) { return ((&b.Edat) == e); });
  if (b == std::end(bosses)) {
    return;
  }

  BitSendCommand(Cmd, Param);
}

// Return remaining bit count
int BossManager::GetBitLeft() const { return BitGetNum(); }
