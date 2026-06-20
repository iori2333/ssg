#include "autoplay.h"
#include "bullet/bullet.h"
#include "bullet/laser_manager.h"
#include "core/gian.h"
#include "enemy/boss.h"
#include "enemy/boss_manager.h"
#include "enemy/enemy.h"
#include "enemy/enemy_manager.h"
#include "game/ut_math.h"
#include "player/item.h"
#include "player/item_manager.h"
#include "player/player.h"
#include <algorithm>

AutoPlayController AutoPlay;

void AutoPlayController::SetDifficulty(int level) {
  difficulty_ = std::clamp(level, DIFFICULTY_EASY, DIFFICULTY_HARD);
}

int AutoPlayController::GetPredictFrames() const {
  switch (difficulty_) {
  case DIFFICULTY_EASY:
    return 6;
  case DIFFICULTY_HARD:
    return 4;
  default:
    return 5;
  }
}

int AutoPlayController::GetDangerThreshold() const {
  switch (difficulty_) {
  case DIFFICULTY_EASY:
    return 5;
  case DIFFICULTY_HARD:
    return 2;
  default:
    return 3;
  }
}

int AutoPlayController::GetBombRadius() const {
  switch (difficulty_) {
  case DIFFICULTY_EASY:
    return 80 * 64;
  case DIFFICULTY_HARD:
    return 35 * 64;
  default:
    return 55 * 64;
  }
}

int AutoPlayController::GetBombThreshold() const {
  switch (difficulty_) {
  case DIFFICULTY_EASY:
    return 3;
  case DIFFICULTY_HARD:
    return 8;
  default:
    return 5;
  }
}

int AutoPlayController::GetGrazeRange() const {
  switch (difficulty_) {
  case DIFFICULTY_EASY:
    return 75 * 64;
  case DIFFICULTY_HARD:
    return 45 * 64;
  default:
    return 60 * 64;
  }
}

int AutoPlayController::GetPlayerSpeed(bool focused) const {
  static constexpr int speed_tbl[] = {
      (64 * 15),
      (64 * 18),
      (64 * 21),
  };
  int spd = speed_tbl[std::min<int>(Players.Weapon(), 2)];
  if (focused) {
    spd /= 3;
  }
  return spd;
}

int AutoPlayController::GetStepX(int dir, int speed) {
  switch (dir) {
  case 0:
    return 0;
  case 3:
    return -(speed >> 2);
  case 4:
    return (speed >> 2);
  case 5:
    return -(speed / 6);
  case 6:
    return (speed / 6);
  case 7:
    return -(speed / 6);
  case 8:
    return (speed / 6);
  default:
    return 0;
  }
}

int AutoPlayController::GetStepY(int dir, int speed) {
  switch (dir) {
  case 0:
    return 0;
  case 1:
    return -(speed >> 2);
  case 2:
    return (speed >> 2);
  case 5:
    return -(speed / 6);
  case 6:
    return -(speed / 6);
  case 7:
    return (speed / 6);
  case 8:
    return (speed / 6);
  default:
    return 0;
  }
}

INPUT_BITS AutoPlayController::Update() {
  frame_counter_ = (frame_counter_ + 1) % 5;

  if (Players.IsGameOver()) {
    return 0;
  }

  int speed = GetPlayerSpeed(false);
  int player_x = Players.X();
  int player_y = Players.Y();

  std::vector<BulletPrediction> predictions;
  BuildPredictions(predictions);

  int best_score;
  int best_dir = FindBestDirection(predictions, player_x, player_y, speed,
                                   best_score);

  bool focused = ShouldFocus(best_score);
  if (focused) {
    speed = GetPlayerSpeed(true);
    best_dir = FindBestDirection(predictions, player_x, player_y, speed,
                                 best_score);
  }

  if (best_score <= GetDangerThreshold() && ShouldBomb(predictions, player_x,
                                                        player_y, speed)) {
    bomb_this_frame_ = true;
  }

  if (best_score >= 5) {
    TargetPoint enemy = FindNearestEnemy();
    if (enemy.valid) {
      int dx_to_enemy = enemy.x - player_x;
      int dy_to_enemy = enemy.y - player_y;

      bool in_attack_position =
          (dy_to_enemy < -(40 * 64)) && (dy_to_enemy > -(120 * 64)) &&
          (std::abs(dx_to_enemy) < (50 * 64));

      if (!in_attack_position) {
        int target_y = enemy.y + (80 * 64);
        if (dy_to_enemy < -(120 * 64)) {
          target_y = enemy.y - (40 * 64);
        }

        int enemy_dir = DirectionToward(player_x, player_y, enemy.x, target_y);
        if (enemy_dir != best_dir && enemy_dir != 0) {
          int enemy_score = EvaluateCandidate(predictions, enemy_dir, player_x,
                                              player_y, speed);
          if (enemy_score >= 4) {
            best_dir = enemy_dir;
            best_score = enemy_score;
          }
        }
      }
    }
  }

  if (best_dir != prev_dir_ && best_score == prev_dir_ &&
      EvaluateCandidate(predictions, prev_dir_, player_x, player_y, speed) >=
          best_score) {
    best_dir = prev_dir_;
  }
  prev_dir_ = best_dir;

  INPUT_BITS keys = 0;

  switch (best_dir) {
  case 1:
    keys |= KEY_UP;
    break;
  case 2:
    keys |= KEY_DOWN;
    break;
  case 3:
    keys |= KEY_LEFT;
    break;
  case 4:
    keys |= KEY_RIGHT;
    break;
  case 5:
    keys |= KEY_ULEFT;
    break;
  case 6:
    keys |= KEY_URIGHT;
    break;
  case 7:
    keys |= KEY_DLEFT;
    break;
  case 8:
    keys |= KEY_DRIGHT;
    break;
  default:
    break;
  }

  if (focused) {
    keys |= KEY_SHIFT;
  }

  if (bomb_this_frame_) {
    keys |= KEY_BOMB;
    bomb_this_frame_ = false;
  }

  SteerTowardItems(keys);

  if (frame_counter_ < 4) {
    keys |= KEY_TAMA;
  }

  return keys;
}

void AutoPlayController::BuildPredictions(
    std::vector<BulletPrediction> &predictions) {
  int frames = GetPredictFrames();

  auto scan = [&](uint16_t count,
                  const std::array<uint16_t, TAMA_MAX> &indices) {
    for (uint16_t i = 0; i < count; i++) {
      uint16_t idx = indices[i];
      const Bullet &b = Bullets.bullets[idx];
      if ((b.flag & TF_DELETE) != 0) {
        continue;
      }
      BulletPrediction p;
      for (int t = 0; t < PREDICT_FRAMES; t++) {
        int f = t + 1;
        if (t < static_cast<int>(frames)) {
          p.x[t] = b.x + b.vx * f;
          p.y[t] = b.y + b.vy * f;
        } else {
          p.x[t] = p.x[frames - 1];
          p.y[t] = p.y[frames - 1];
        }
      }
      predictions.push_back(p);
    }
  };

  scan(Bullets.count_small, Bullets.indices_small);
  scan(Bullets.count_large, Bullets.indices_large);
}

int AutoPlayController::EvaluateCandidate(
    const std::vector<BulletPrediction> &predictions, int dir, int player_x,
    int player_y, int speed) {

  int step_x = GetStepX(dir, speed);
  int step_y = GetStepY(dir, speed);

  int frames = GetPredictFrames();

  for (int t = 0; t < frames; t++) {
    int f = t + 1;
    int px = player_x + step_x * f;
    int py = player_y + step_y * f;

    if (px < SX_MIN || px > SX_MAX || py < SY_MIN || py > SY_MAX) {
      return t;
    }

    for (const auto &p : predictions) {
      int dx = px - p.x[t];
      int dy = py - p.y[t];
      if (std::abs(dx) < COLLISION_X && std::abs(dy) < COLLISION_Y) {
        return t;
      }
    }

    auto enemy_hits = [&](int ex, int ey, int vx, int vy,
                          uint16_t g_width, uint16_t g_height) -> bool {
      int epx = ex + vx * f;
      int epy = ey + vy * f;
      int cx = static_cast<int>(g_width) + SAFETY_MARGIN_X;
      int cy = static_cast<int>(g_height) + SAFETY_MARGIN_Y;
      return std::abs(px - epx) < cx && std::abs(py - epy) < cy;
    };

    for (uint16_t i = 0; i < Enemies.count; i++) {
      uint16_t idx = Enemies.indices[i];
      const EnemyData &e = Enemies.entities[idx];
      if ((e.flag & EF_DELETE) != 0) {
        continue;
      }
      if ((e.flag & EF_HITSB) == 0) {
        continue;
      }
      if (enemy_hits(e.x, e.y, e.vx, e.vy, e.g_width, e.g_height)) {
        return t;
      }
    }

    for (uint16_t i = 0; i < Bosses.count; i++) {
      const BossData &b = Bosses.bosses[i];
      if (!b.IsUsed) {
        continue;
      }
      if ((b.Edat.flag & EF_DELETE) != 0) {
        continue;
      }
      if ((b.Edat.flag & EF_HITSB) == 0) {
        continue;
      }
      if (enemy_hits(b.Edat.x, b.Edat.y, b.Edat.vx, b.Edat.vy, b.Edat.g_width,
                     b.Edat.g_height)) {
        return t;
      }
    }

    // --- Short / reflect laser collision ---
    for (uint16_t i = 0; i < Lasers.count; i++) {
      uint16_t idx = Lasers.laser_indices[i];
      const LASER_DATA &lp = Lasers.lasers[idx];
      if ((lp.flag & LF_DELETE) != 0) {
        continue;
      }
      int lx = lp.x + lp.vx * f;
      int ly = lp.y + lp.vy * f;
      int ltx = px - lx;
      int lty = py - ly;
      int lproj = cosl(lp.d, ltx) + sinl(lp.d, lty);
      int lperp = std::abs(-sinl(lp.d, ltx) + cosl(lp.d, lty));
      int llen = std::min(lp.l + lp.v * f, lp.lmax);
      int lwid = std::max(lp.w, lp.wmax) + SAFETY_MARGIN_X;
      if (lproj > 0 && lproj <= llen && lperp <= lwid) {
        return t;
      }
    }

    // --- Long laser collision ---
    for (int i = 0; i < LLASER_MAX; i++) {
      const LongLaserData &lp = Lasers.long_lasers[i];
      if (lp.flag != LLF_OPEN && lp.flag != LLF_NORM) {
        continue;
      }
      int ltx = px - lp.x;
      int lty = py - lp.y;
      int lproj = cosl(lp.d, ltx) + sinl(lp.d, lty);
      int lperp = std::abs(-sinl(lp.d, ltx) + cosl(lp.d, lty));
      int lwid = lp.w + SAFETY_MARGIN_X;
      if (lproj > 0 && lperp <= lwid) {
        return t;
      }
    }

    // --- Homing laser collision ---
    static constexpr int HOMINGL_WIDTH = 8 * 64;
    static constexpr int HOMINGL_TRAIL = 7 * 4;
    const HomingLaserData *hl = Lasers.active.Next;
    while (hl != nullptr) {
      if (hl->State != HLS_DEAD) {
        int ci = hl->Current;
        for (int j = 0; j < HOMINGL_TRAIL; j++) {
          int vx = hl->p[ci].x;
          int vy = hl->p[ci].y;
          int hwid = (HOMINGL_WIDTH * 2 / 3) + SAFETY_MARGIN_X;
          if (std::abs(px - vx) < hwid && std::abs(py - vy) < hwid) {
            return t;
          }
          ci = (ci + 1) % HOMINGL_TRAIL;
        }
      }
      hl = hl->Next;
    }
  }

  return frames;
}

int AutoPlayController::FindBestDirection(
    const std::vector<BulletPrediction> &predictions, int player_x,
    int player_y, int speed, int &best_score) {

  int best_dir = 0;
  best_score = -1;

  int scores[NUM_CANDIDATES];

  for (int dir = 0; dir < NUM_CANDIDATES; dir++) {
    scores[dir] = EvaluateCandidate(predictions, dir, player_x, player_y,
                                    speed);
    if (scores[dir] > best_score ||
        (scores[dir] == best_score && dir == prev_dir_)) {
      best_score = scores[dir];
      best_dir = dir;
    }
  }

  int home_x = (SX_MIN + SX_MAX) / 2;
  int home_y = SY_MAX - (10 * 64);
  int home_dir = DirectionToward(player_x, player_y, home_x, home_y);

  if (home_dir != 0 && home_dir != best_dir) {
    if (scores[home_dir] >= best_score) {
      best_dir = home_dir;
      best_score = scores[home_dir];
    } else if (best_score >= GetPredictFrames() &&
               scores[home_dir] >= GetPredictFrames() - 1) {
      best_dir = home_dir;
      best_score = scores[home_dir];
    }
  }

  return best_dir;
}

AutoPlayController::TargetPoint AutoPlayController::FindNearestEnemy() {
  TargetPoint result{0, 0, false};
  int nearest_dist_sq = (1 << 30);

  int player_x = Players.X();
  int player_y = Players.Y();

  for (uint16_t i = 0; i < Enemies.count; i++) {
    uint16_t idx = Enemies.indices[i];
    const EnemyData &e = Enemies.entities[idx];
    if ((e.flag & EF_DELETE) != 0) {
      continue;
    }
    if ((e.flag & EF_DAMAGE) == 0) {
      continue;
    }
    int dx = e.x - player_x;
    int dy = e.y - player_y;
    int dist_sq = dx * dx + dy * dy;
    if (dist_sq < nearest_dist_sq) {
      nearest_dist_sq = dist_sq;
      result.x = e.x;
      result.y = e.y;
      result.valid = true;
    }
  }

  for (uint16_t i = 0; i < Bosses.count; i++) {
    const BossData &b = Bosses.bosses[i];
    if (!b.IsUsed) {
      continue;
    }
    if ((b.Edat.flag & EF_DELETE) != 0) {
      continue;
    }
    int dx = b.Edat.x - player_x;
    int dy = b.Edat.y - player_y;
    int dist_sq = dx * dx + dy * dy;
    if (dist_sq < nearest_dist_sq) {
      nearest_dist_sq = dist_sq;
      result.x = b.Edat.x;
      result.y = b.Edat.y;
      result.valid = true;
    }
  }

  return result;
}

int AutoPlayController::DirectionToward(int player_x, int player_y,
                                        int target_x, int target_y) {
  int dx = target_x - player_x;
  int dy = target_y - player_y;

  int adx = std::abs(dx);
  int ady = std::abs(dy);

  if (adx < 20 * 64 && ady < 20 * 64) {
    return 0;
  }

  if (adx > ady * 2) {
    return (dx > 0) ? 4 : 3;
  }
  if (ady > adx * 2) {
    return (dy > 0) ? 2 : 1;
  }

  if (dx > 0 && dy > 0) {
    return 8;
  }
  if (dx > 0 && dy < 0) {
    return 6;
  }
  if (dx < 0 && dy > 0) {
    return 7;
  }
  return 5;
}

bool AutoPlayController::ShouldFocus(int best_score) {
  return best_score <= GetDangerThreshold();
}

bool AutoPlayController::ShouldBomb(
    const std::vector<BulletPrediction> &predictions, int player_x,
    int player_y, int speed) {
  if (!Players.Bombs() || Players.IsInvincible() || Players.IsBombActive()) {
    return false;
  }

  int bomb_radius = GetBombRadius();
  int near_count = 0;
  bool very_close = false;

  for (const auto &p : predictions) {
    int dx = p.x[0] - player_x;
    int dy = p.y[0] - player_y;
    int dist_sq = dx * dx + dy * dy;
    int threshold = bomb_radius * bomb_radius;
    if (dist_sq < threshold) {
      near_count++;
      if (dist_sq < threshold / 4) {
        very_close = true;
      }
    }
  }

  int bomb_threshold = GetBombThreshold();
  return (near_count >= bomb_threshold) ||
         (very_close && near_count >= bomb_threshold / 2);
}

void AutoPlayController::SteerTowardItems(INPUT_BITS &keys) {
  if (keys != 0 && keys != KEY_TAMA) {
    return;
  }

  int player_x = Players.X();
  int player_y = Players.Y();

  int nearest_dist = (1 << 30);
  int nearest_x = 0;
  int nearest_y = 0;
  bool found = false;

  for (uint16_t i = 0; i < Items.count; i++) {
    uint16_t idx = Items.indices[i];
    const ItemData &item = Items.entities[idx];
    if (item.type == ITEM_DELETE) {
      continue;
    }

    int dx = item.x - player_x;
    int dy = item.y - player_y;
    int dist = std::abs(dx) + std::abs(dy);

    int priority_bonus = 0;
    if (item.type == ITEM_BOMB) {
      priority_bonus = -(120 * 64);
    } else if (item.type == ITEM_EXTEND) {
      priority_bonus = -(90 * 64);
    }

    dist += priority_bonus;

    if (dist < nearest_dist) {
      nearest_dist = dist;
      nearest_x = item.x;
      nearest_y = item.y;
      found = true;
    }
  }

  if (!found) {
    return;
  }

  int dx = nearest_x - player_x;
  int dy = nearest_y - player_y;

  int threshold = 120 * 64;
  if (std::abs(dx) < threshold && std::abs(dy) < threshold) {
    int dir = DirectionToward(player_x, player_y, nearest_x, nearest_y);
    switch (dir) {
    case 1:
      keys |= KEY_UP;
      break;
    case 2:
      keys |= KEY_DOWN;
      break;
    case 3:
      keys |= KEY_LEFT;
      break;
    case 4:
      keys |= KEY_RIGHT;
      break;
    case 5:
      keys |= KEY_ULEFT;
      break;
    case 6:
      keys |= KEY_URIGHT;
      break;
    case 7:
      keys |= KEY_DLEFT;
      break;
    case 8:
      keys |= KEY_DRIGHT;
      break;
    default:
      break;
    }
  }
}

INPUT_BITS AutoPlayController::DirectionToKeys(int dx, int dy) {
  const int dead = 64 * 2;

  int adx = std::abs(dx);
  int ady = std::abs(dy);

  if (adx < dead && ady < dead) {
    return 0;
  }

  INPUT_BITS keys = 0;

  if (dy < -dead) {
    keys |= KEY_UP;
  } else if (dy > dead) {
    keys |= KEY_DOWN;
  }

  if (dx < -dead) {
    keys |= KEY_LEFT;
  } else if (dx > dead) {
    keys |= KEY_RIGHT;
  }

  return keys;
}
