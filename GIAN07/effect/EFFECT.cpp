/*                                                                           */
/*   EFFECT.cpp   エフェクト管理                                             */
/*                                                                           */
/*                                                                           */

#include "EFFECT.h"
#include "FONTUTY.h"
#include "GEOMETRY.h"
#include "GIAN.h"
#include "game/cast.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/text_backend.h"

// Effects.string_effects[], Effects.circle_effects[], Effects.lock_info[], Effects.screen_info, Effects.mtitle_rect, Effects.mtitle_strs[]
// → effect_manager.cpp の EffectManager に移動

// 円エフェクトの初期化 //
void EffectManager::InitCircleEffects() {
  for (auto &it : Effects.circle_effects) {
    it.type = CEFC_NONE;
  }
}

// 円エフェクトを動かす //
void EffectManager::MoveCircleEffects() {
  for (auto &it : Effects.circle_effects) {
    auto *ce = &it;
    ce->count++;

    switch (ce->type) {
    case (CEFC_STAR):
      ce->r -= 3;
      ce->d += 2;
      if (ce->r <= 0) {
        ce->type = CEFC_NONE;
      }
      break;

    case (CEFC_CIRCLE1): // 集
      ce->r -= (10 + 5);
      if (ce->r <= 0) {
        ce->type = CEFC_NONE;
      }
      break;

    case (CEFC_CIRCLE2): // 離
      ce->r += (8 + 5);
      if (ce->r >= ce->rmax) {
        ce->type = CEFC_NONE;
      }
      break;
    }
  }
}

// 円エフェクトを描画する //
void EffectManager::DrawCircleEffects() {
  int j, r;
  int x1, x2, y1, y2;
  static constexpr uint8_t dtable[4] = {0, 1, 3, 7};

  GrpGeom->Lock();

  for (const auto &it : Effects.circle_effects) {
    const auto *ce = &it;
    switch (ce->type) {
    case (CEFC_STAR):
      for (uint8_t k = 0; k < 4; k++) {
        r = ce->r - k * 7;
        if (r < 0)
          continue;
        GrpGeom->SetColor({5u, (k + 2u), (k + 2u)});
        for (j = 0; j < 5; j++) {
          x1 =
              ce->x + cosl(ce->d + dtable[k] * ce->count / 10 + j * 256 / 5, r);
          y1 =
              ce->y + sinl(ce->d + dtable[k] * ce->count / 10 + j * 256 / 5, r);
          x2 = ce->x +
               cosl(ce->d + dtable[k] * ce->count / 10 + (j + 2) * 256 / 5, r);
          y2 = ce->y +
               sinl(ce->d + dtable[k] * ce->count / 10 + (j + 2) * 256 / 5, r);
          GrpGeom->DrawLine(x1, y1, x2, y2);
        }
      }
      break;

    case (CEFC_CIRCLE1): // 集
      for (uint8_t k = 0; k < 4; k++) {
        r = ce->r - max(2, (k * ce->r) / 8);
        if (r < 0)
          continue;
        GrpGeom->SetColor({5u, (k + 2u), (k + 2u)});
        GeomCircle({ce->x, ce->y}, r);
      }
      break;

    case (CEFC_CIRCLE2): // 離
      for (uint8_t k = 0; k < 4; k++) {
        r = ce->r - max(2, (k * ce->r) / 12);
        // r = ce->r - max(2, (k * (600-ce->r))/16);
        if (r < 0)
          continue;
        GrpGeom->SetColor({5u, (k + 2u), (k + 2u)});
        GeomCircle({ce->x, ce->y}, r);
      }
      break;
    }
  }

  GrpGeom->Unlock();
}

// 円エフェクトをセットする //
void EffectManager::SpawnCircleEffect(int x, int y, uint8_t type) {
  auto ce = std::ranges::find_if(
      Effects.circle_effects, [](const auto &ce) { return (ce.type == CEFC_NONE); });
  if (ce == std::end(Effects.circle_effects)) {
    return;
  }

  ce->x = x >> 6;
  ce->y = y >> 6;
  ce->type = type;
  ce->count = ce->d = 0;

  switch (type) {
  case (CEFC_STAR):
    Snd_SEPlay(SOUND_ID_TAMEFAST);
    ce->rmax = ce->r = 400;
    break;

  case (CEFC_CIRCLE1): // 集
    ce->rmax = ce->r = 600 + 50;
    break;

  case (CEFC_CIRCLE2): // 離
    ce->rmax = 900 - 100;
    ce->r = 0;
    break;

  default:
    break;
  }
}

void EffectManager::InitMusicTitle() {
  Effects.mtitle_rect = TextObj.Register({((X_MAX + 1) - X_MIN), 20});
}

// エフェクトの初期化を行う //
void EffectManager::InitStringEffects() {
  for (auto &it : Effects.string_effects) {
    // memset(Effects.string_effects+i,0,sizeof(SEFFECT_DATA));
    it.cmd = SEFC_NONE;
  }
}

// 文字列系エフェクト //
void EffectManager::SpawnStringEffect(int x, int y, const char *s) {
  int i, j, len;

  len = strlen(s);
  for (i = j = 0; i < len; i++) {
    while (Effects.string_effects[j].cmd != SEFC_NONE) {
      j++;
      if (j >= SEFFECT_MAX)
        return;
    }
    Effects.string_effects[j].c = s[i];
    Effects.string_effects[j].x = (x + (i << 4) + 512) << 6;
    Effects.string_effects[j].y = (y) << 6;
    Effects.string_effects[j].vx = (-20) << 6;
    Effects.string_effects[j].vy = (0) << 6;
    Effects.string_effects[j].cmd = SEFC_STR1;
    Effects.string_effects[j].time = 26;
  }
}

// 得点表示エフェクト //
void EffectManager::SpawnPointEffect(int x, int y, uint32_t point) {
  for (auto &it : Effects.string_effects) {
    if (it.cmd != SEFC_NONE) {
      continue;
    }

    it.point = point;
    it.x = x;
    it.y = y;
    it.vx = 0;
    it.vy = ((-64 * 3) + 32);
    it.cmd = SEFC_STR2;
    it.time = 90;
    return;
  }
}

// ゲームオーバーの表示 //
void EffectManager::SpawnGameOverEffect() {
  for (auto &it : Effects.string_effects) {
    if (it.cmd != SEFC_NONE) {
      continue;
    }

    it.x = GX_MID;
    it.y = (GY_MID - (64 * (60 + 40)));
    it.vx = 0;
    it.vy = 0;
    it.cmd = SEFC_GAMEOVER;
    it.time = 120 - 35; // 100;
    return;
  }
}

// 曲名の表示 //
void EffectManager::SetMusicTitle(int y, Narrow::string_view s) {
  // 空きバッファ検索 //
  auto e = std::ranges::find_if(
      Effects.string_effects, [](const auto &e) { return (e.cmd == SEFC_NONE); });
  if (e == std::end(Effects.string_effects)) {
    return;
  }

  Effects.mtitle_strs[1] = s;
  PIXEL_SIZE extent = {0, 0};
  for (const auto s : Effects.mtitle_strs) {
    const auto s_extent = TextObj.TextExtent(FONT_ID::NORMAL, s);
    extent.w += s_extent.w;
    extent.h = s_extent.h;
  };

  // 曲のタイトルが長すぎる場合、どうしましょう？ //
  auto x = (std::max)((640 - 128 - 32 - extent.w), 128);

  e->cmd = SEFC_MTITLE1;
  e->x = (x << 6); // + ((64 * 2) * 16);
  e->y = (y << 6);
  e->time = (64 * 2);
  e->vx = extent.w;
  e->vy = extent.h;
}

void EffectManager::RenderMusicTitle(WINDOW_POINT topleft, const PIXEL_LTWH &subrect) {
  const auto mtitle = Effects.mtitle_strs[1];
  TextObj.Render(
      topleft, Effects.mtitle_rect, mtitle,
      [](TEXTRENDER_SESSION &s) {
        const auto gradient_func = [](PIXEL_COORD y) -> uint8_t {
          return (255 + 8 - (y * 8));
        };
        DrawGrdFont(s, Effects.mtitle_strs, FONT_ID::NORMAL, true, gradient_func);
      },
      subrect);
}

// エフェクトを動かす(仕様変更の可能性があります) //
void EffectManager::MoveStringEffects() {
  for (auto &it : Effects.string_effects) {
    auto *e = &it;
    switch (e->cmd) {
    case (SEFC_STR1):
      e->x += e->vx;
      e->y += e->vy;
      if (e->time == 0) {
        e->cmd = SEFC_STR1_2;
        e->time = 256; // 128;
      }
      break;

    case (SEFC_STR1_3):
      e->x += e->vx;
      e->y += (e->vy += 16);
      if (e->time == 0)
        e->cmd = SEFC_NONE;
      break;

    case (SEFC_STR1_2):
      if (e->time == 0) {
        const uint8_t deg = (128 + (rnd() % 128));
        e->cmd = SEFC_STR1_3;
        e->time = 64;
        e->vx = cosl(deg, 10 * 64);
        e->vy = sinl(deg, 10 * 64);
      }
      break;

    case (SEFC_STR2):
      if (e->time == 0)
        e->cmd = SEFC_NONE;
      e->x += e->vx;
      e->y += (e->vy += 3);
      break;

    case (SEFC_GAMEOVER):
      if (e->time == 0) {
        e->cmd = SEFC_GAMEOVER2;
        e->time = 35;
      }
      break;

    case (SEFC_MTITLE1): // 曲名出現
      // e->x -= 16;
      if (e->time == 0) {
        e->cmd = SEFC_MTITLE2;
        e->time = 64 * 4;
      }
      break;

    case (SEFC_MTITLE2): // 曲名停止
      if (e->time == 0) {
        e->cmd = SEFC_MTITLE3;
        e->time = 64 * 2;
      }
      break;

    case (SEFC_MTITLE3): // 曲名消去
      e->x += 64;
      if (e->time == 0) {
        e->cmd = SEFC_NONE;
      }
      break;

    case (SEFC_NONE):
    default:
      break;
    }
    e->time--;
  }
}

// エフェクトを描画する(仕様変更の可能性があります) //
void EffectManager::DrawStringEffects() {
  int j, k;
  int x, y;
  int temp;
  PIXEL_LTWH src;
  char buf[20];

  for (const auto &it : Effects.string_effects) {
    const auto *e = &it;
    switch (e->cmd) {
    case (SEFC_STR1):
    case (SEFC_STR1_2):
    case (SEFC_STR1_3):
      GrpPutc(e->x >> 6, e->y >> 6, e->c);
      break;

    case (SEFC_STR2):
      sprintf(buf, "%u", e->point);
      GrpPutScore(e->x >> 6, e->y >> 6, buf);
      break;

    case (SEFC_GAMEOVER):
      strcpy(buf, "GAME OVER");
      for (j = 0; j < 9; j++) {
        // *37 //
        const auto angle = Cast::down<uint8_t>((e->time * 3) + (j * 26));
        x = (e->x >> 6) + cosl(angle, e->time * 4);
        y = (e->y >> 6) + sinl(angle, e->time * 4);
        GrpPutc(x, y, buf[j]);
      }
      break;

    case (SEFC_GAMEOVER2):
      x = (e->x >> 6) + 8;
      y = (e->y >> 6) + 8;
      j = (35 - e->time) / 2;
      GrpGeom->Lock();
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->SetAlphaNorm(Cast::down<uint8_t>((35 - e->time) * 3));
      GrpGeom->DrawBoxA((x - 170), (y - j), (x + 170), (y + j));
      GrpGeom->Unlock();

      strcpy(buf, "GAME OVER");
      for (j = 0; j < 9; j++) {
        // *37 //
        x = (e->x >> 6) + (j - 4) * (35 - e->time);
        y = (e->y >> 6);
        GrpPutc(x, y, buf[j]);
      }
      break;

    case (SEFC_MTITLE3): {
      const auto degx = Cast::down<uint8_t>((64 * 2) - e->time);
      for (j = 0; j < e->vx; j++) {
        src = {j, 0, 1, e->vy};
        temp = sinl(degx, 100);
        const auto y = ((e->y >> 6) - sinl((degx + j), degx /*40*/));
        for (k = 0; k < 2; k++) {
          const auto x = ((e->x >> 6) + sinl((degx + (j / 2)), temp) + j + k);
          Effects.RenderMusicTitle({x, y}, src);
        }
      }
    } break;

    case (SEFC_MTITLE1): {
      const auto degx = Cast::down<uint8_t>(e->time);
      for (j = 0; j < e->vx; j++) {
        src = {j, 0, 1, e->vy};
        temp = sinl(degx, 160);
        const auto y = ((e->y >> 6) - sinl((degx + j), degx /*40*/));
        for (k = 0; k < 2; k++) {
          const auto x = ((e->x >> 6) + sinl((degx + (j / 2)), temp) + j + k);
          Effects.RenderMusicTitle({x, y}, src);
        }
      }
    } break;

    case (SEFC_MTITLE2): {
      GrpGeom->Lock();
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->SetAlphaNorm((sinl(Cast::down<uint8_t>(e->time - 32), 80) + 80));
      for (j = 0; j < 16; j++) {
        temp = sinl(128 + j * 16, 16);
        GrpGeom->DrawBoxA(((e->x >> 6) + temp - 16), ((e->y >> 6) + j),
                          ((X_MAX - 16) - temp), ((e->y >> 6) + j + 1));
      }
      GrpGeom->Unlock();
      src = {0, 0, e->vx, e->vy};
      Effects.RenderMusicTitle({(e->x >> 6), (e->y >> 6)}, src);
    } break;

    case (SEFC_NONE):
    default:
      break;
    }
  }
}

// 画面全体に対するエフェクトの初期化 //
void EffectManager::InitScreenEffect() {
  Effects.screen_info.cmd = SCNEFC_NONE;
  Effects.screen_info.count = 0;

  GrpBackend_SetClip(PLAYFIELD_CLIP);
}

// 画面全体に対するエフェクトをセットする //
void EffectManager::SetScreenEffect(uint8_t cmd) {
  // if(Effects.screen_info.cmd != SCNEFC_NONE) return;

  Effects.screen_info.cmd = cmd;
  Effects.screen_info.count = 0;

  switch (cmd) {
  case (SCNEFC_CFADEIN):  // 円形フェードイン
  case (SCNEFC_CFADEOUT): // 円形フェードアウト
    break;

  case (SCNEFC_WHITEIN):  // ホワイトイン
  case (SCNEFC_WHITEOUT): // ホワイトアウト
    break;

  default:                        // ばぐばぐ
    Effects.screen_info.cmd = SCNEFC_NONE; // 一応エフェクトを切っておく
    break;
  }
}

// 画面全体に対するエフェクトを動かす //
void EffectManager::MoveScreenEffect() {
  switch (Effects.screen_info.cmd) {
  case (SCNEFC_CFADEIN): // 円形フェードイン
    Effects.screen_info.count += 10;
    if (Effects.screen_info.count > 600)
      Effects.screen_info.cmd = SCNEFC_NONE;
    break;

  case (SCNEFC_CFADEOUT): // 円形フェードアウト
    Effects.screen_info.count += 10;
    if (Effects.screen_info.count > 600) {
      Effects.screen_info.count = 600;
      // Effects.screen_info.cmd = SCNEFC_NONE;
    }
    break;

  case (SCNEFC_WHITEIN): // ホワイトイン
    Effects.screen_info.count += 10;
    if (Effects.screen_info.count >= 160) {
      // Effects.screen_info.cmd   = SCNEFC_WHITEOUT;
      // Effects.screen_info.count = 0;
      Effects.screen_info.count = 150;
    }
    break;

  case (SCNEFC_WHITEOUT): // ホワイトアウト
    Effects.screen_info.count += 10;
    if (Effects.screen_info.count >= 160)
      Effects.screen_info.cmd = SCNEFC_NONE;
    break;

  default: // ばぐばぐ
    break;
  }
}

// 画面全体に対するエフェクトを描画する //
void EffectManager::DrawScreenEffect() {
  int i, j;
  PIXEL_LTRB src;

  switch (Effects.screen_info.cmd) {
  case (SCNEFC_CFADEIN): // 円形フェードイン
    CircleFadeOut(X_MID, Y_MID, Effects.screen_info.count);
    break;

  case (SCNEFC_CFADEOUT): // 円形フェードアウト
    CircleFadeOut(X_MID, Y_MID, 400 - Effects.screen_info.count);
    break;

  case (SCNEFC_WHITEIN): // ホワイトイン
    src = PIXEL_LTWH{((15 - (Cast::sign<int>(Effects.screen_info.count) / 10)) * 16),
                     (128 + 16), 16, 16};
    for (i = 128; i < 640 - 128; i += 16) {
      for (j = 0; j < 480; j += 16) {
        GrpSurface_Blit({i, j}, SURFACE_ID::SYSTEM, src);
      }
    }
    break;

  case (SCNEFC_WHITEOUT): // ホワイトアウト
    src = PIXEL_LTWH{((Cast::sign<int>(Effects.screen_info.count) / 10) * 16),
                     (128 + 16), 16, 16};
    for (i = 128; i < 640 - 128; i += 16) {
      for (j = 0; j < 480; j += 16) {
        GrpSurface_Blit({i, j}, SURFACE_ID::SYSTEM, src);
      }
    }
    break;

  default: // ばぐばぐ
    break;
  }
}

// 円形フェードサポート関数 //
void EffectManager::CircleFadeOut(int x, int y, int r) {
  PIXEL_LTRB src;
  int temp;

  if (r < 0)
    r = 0;

  for (auto i = 0; i < GRP_RES.w; i += 16) {
    for (auto j = 0; j < GRP_RES.h; j += 16) {
      temp = isqrt((i - x) * (i - x) + (j - y) * (j - y));
      if (temp < r && r - temp < 8 * 16 && temp >= 0) {
        temp = (r - temp) >> 3;
        // temp = (r-temp)>>4;
        // src = PIXEL_LTWH{ ((temp << 4) + 256), 104, 16, 16 };
        src = PIXEL_LTWH{(temp << 4), 128, 16, 16};
        GrpSurface_Blit({i, j}, SURFACE_ID::SYSTEM, src);
      } else if (temp >= r) {
        // src = PIXEL_LTWH{ 256, 104, 16, 16 };
        src = PIXEL_LTWH{0, 128, 16, 16};
        GrpSurface_Blit({i, j}, SURFACE_ID::SYSTEM, src);
      }
    }
  }

  if (r != 0) {
    const WINDOW_LTRB clip = {std::clamp((x - r), X_MIN, (X_MAX + 1)),
                              (std::max)((y - r), Y_MIN),
                              std::clamp((x + r + 1), X_MIN, (X_MAX + 1)),
                              (std::min)((y + r + 1), (Y_MAX + 1))};
    GrpBackend_SetClip(clip);
  } else {
    GrpBackend_SetClip({X_MID, Y_MID, X_MID, Y_MID});
  }
}

// ロックオン配列を初期化 //
void EffectManager::InitLockOn() {
  for (auto &it : Effects.lock_info) {
    // memset(Effects.lock_info+i,0,sizeof(LOCKON_INFO));
    it.state = LOCKON_NONE;
  }
}

// 何かをロックオンする //
void EffectManager::LockOn(int *x, int *y, int wx64, int hx64) {
  auto l = std::ranges::find_if(
      Effects.lock_info, [](const auto &l) { return (l.state == LOCKON_NONE); });
  if (l == std::end(Effects.lock_info)) {
    return;
  }

  l->x = x;
  l->y = y;

  l->width = (wx64 << 2);
  l->height = (hx64 << 2);

  l->vx = -(wx64 * 4 / 30);
  l->vy = -(hx64 * 4 / 30);
  l->count = 30;

  l->state = LOCKON_01;
  // Snd_SEPlay(SOUND_ID_SELECT);
}

// ロックオンアニメーション動作 //
void EffectManager::MoveLockOn() {
  for (auto &it : Effects.lock_info) {
    auto *l = &it;
    switch (l->state) {
    case (LOCKON_01):
      l->count--;
      l->width += l->vx;
      l->height += l->vy;
      if (l->count == 0) {
        l->state = LOCKON_NONE; // LOCKON_02;
        l->count = 30;          // 120;
      }
      break;
    case (LOCKON_02):
      l->count--;
      if (l->count == 0) {
        l->state = LOCKON_NONE; // LOCKON_03;
        l->count = 30;
      }
      break;
    case (LOCKON_03):
      l->count--;
      l->width -= l->vx;
      l->height -= l->vy;
      if (l->count == 0) {
        l->state = LOCKON_NONE;
      }
      break;
    }
  }
}

// ロックオン枠描画 //
void EffectManager::DrawLockOn() {
  for (const auto &it : Effects.lock_info) {
    const auto *l = &it;
    if (l->state != LOCKON_NONE) {
      GrpSurface_Blit({((*l->x >> 6) - 8), ((*l->y >> 6) - 8)},
                      SURFACE_ID::SYSTEM, {208, 80, 272, 96});

      GrpSurface_Blit(
          {
              (((*l->x - l->width) >> 6) - 4),
              (((*l->y - l->height) >> 6) - 4),
          },
          SURFACE_ID::SYSTEM, {280, 88, 288, 96});
      GrpSurface_Blit(
          {
              (((*l->x + l->width) >> 6) - 4),
              (((*l->y + l->height) >> 6) - 4),
          },
          SURFACE_ID::SYSTEM, {272, 80, 280, 88});

      GrpSurface_Blit(
          {
              (((*l->x - l->width) >> 6) - 4),
              (((*l->y + l->height) >> 6) - 4),
          },
          SURFACE_ID::SYSTEM, {280, 80, 288, 88});
      GrpSurface_Blit(
          {
              (((*l->x + l->width) >> 6) - 4),
              (((*l->y - l->height) >> 6) - 4),
          },
          SURFACE_ID::SYSTEM, {272, 88, 280, 96});
    }
  }
}

// Effects.enable_warn_efc, Effects.warn_efc_time → effect_manager.cpp の EffectManager に移動

// ワーニングの初期化 //
void EffectManager::InitWarningEffect() { Effects.enable_warn_efc = false; }

// ワーニングの発動！！ //
void EffectManager::SetWarningEffect() {
  Effects.enable_warn_efc = true;
  Effects.warn_efc_time = 0;
  Effects.InitWarningText();
}

// ワーニングの動作 //
void EffectManager::MoveWarningEffect() {
  if (!Effects.enable_warn_efc)
    return;

  if (Effects.warn_efc_time < 64 + 128) { // 64+256){
    Effects.MoveWarningText(Cast::down<uint8_t>(Effects.warn_efc_time));
  } else {
    MoveWarningR(-1);
  }

  if ((Effects.warn_efc_time++) == (256 + 10)) {
    Effects.enable_warn_efc = false;
  }
}

// ワーニングの描画 //
void EffectManager::DrawWarningEffect() {
  int r;

  if (!Effects.enable_warn_efc)
    return;

  if (Effects.warn_efc_time < 256 - 20)
    Effects.DrawWarningText();

  if (Effects.warn_efc_time > 256 - 40) {
    GrpGeom->Lock();

    r = (Effects.warn_efc_time - (256 - 40)) * 3;
    GrpGeom->SetColor({1, 1, 5});
    GeomCircle({320, 100}, (r -= 4));
    GrpGeom->SetColor({2, 2, 5});
    GeomCircle({320, 100}, (r -= 4));
    GrpGeom->SetColor({3, 3, 5});
    GeomCircle({320, 100}, (r -= 6));
    GrpGeom->SetColor({4, 4, 5});
    GeomCircle({320, 100}, (r -= 6));
    GrpGeom->SetColor({5, 5, 5});
    GeomCircle({320, 100}, (r -= 8));

    GrpGeom->Unlock();
  }
}
