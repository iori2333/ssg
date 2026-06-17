/*                                                                           */
/*   LLaser.cpp   長いレーザーの処理                                         */
/*                                                                           */
/*                                                                           */

#include "GEOMETRY.h"
#include "LLASER.h"
#include "laser_manager.h"
#include "MAID.h"
#include "player_manager.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

//// レーザー変数２ → laser_manager.cpp に移動
// long_lasers[], LLaserCmd は laser_manager.cpp で定義

//// ローカル関数 ////
// private methods declared in laser_manager.h
// private methods declared in laser_manager.h
// private methods declared in laser_manager.h

bool LaserManager::SpawnLongLaser(uint8_t id) {
  // この部分で空いているレーザーのサーチを行う             //
  // もし、レーザーが見つからなければ、FALSE をリターンする //
  // つまり、その場合は、参照カウントを増加させない         //
  auto lp = std::ranges::find_if(
      long_lasers, [](const auto &lp) { return (lp.flag == LLF_DISABLE); });
  if (lp == std::end(long_lasers)) {
    return false;
  }

  lp->dx = long_cmd.dx;
  lp->dy = long_cmd.dy;
  lp->e = long_cmd.e;

  lp->EnemyID = id;

  lp->x = lp->e->x + lp->dx;
  lp->y = lp->e->y + lp->dy;

  lp->v = long_cmd.v;

  lp->c = long_cmd.c;

  lp->lx = 0;
  lp->ly = 0;
  lp->wx = 0;
  lp->wy = 0;

  lp->w = 0;
  lp->wmax = long_cmd.w;

  lp->d = long_cmd.d;

  if (long_cmd.type == LLS_LONGZ) {
    lp->d += atan8(Players.viv.x - lp->x, Players.viv.y - lp->y);
    lp->type = LLS_LONG;
  } else
    lp->type = long_cmd.type;

  lp->infx = cosl(lp->d, 800);
  lp->infy = sinl(lp->d, 800);

  lp->count = 0;

  SetLongPoint(&*lp); // p[4] をセット

  lp->flag = LLF_LINE;

  // Snd_SEPlay(1, LaserCmd.x);

  return true;
}

void LaserManager::OpenLong(const EnemyData *e, uint8_t id) {
  for (auto &it : long_lasers) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL) &&
        lp->flag != LLF_DISABLE) {
      lp->flag = LLF_OPEN;
      Snd_SEPlay(2, lp->x, true);
    }
  }
}

void LaserManager::CloseLong(const EnemyData *e, uint8_t id) {
  if (id == ECLCST_LLASERALL) {
    ForceCloseLong(e);
    return;
  }

  for (auto &it : long_lasers) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id)) {
      lp->flag = LLF_CLOSE;
      Snd_SEStop(2);
    }
  }
}

void LaserManager::LineLong(const EnemyData *e, uint8_t id) {
  for (auto &it : long_lasers) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->flag = LLF_CLOSEL;
      Snd_SEStop(2);
    }
  }
}

void LaserManager::UpdateLongXY(int id) {
  // 注意！！この関数のid は旧式のid の意味を持つことに注意 //

  long_lasers[id].x = long_lasers[id].e->x + long_lasers[id].dx;
  long_lasers[id].y = long_lasers[id].e->y + long_lasers[id].dy;

  SetLongPoint(&long_lasers[id]); // p[4] をセット
}

void LaserManager::RotateLongAbs(const EnemyData *e, uint8_t d, uint8_t id) {
  for (auto &it : long_lasers) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->d = d;

      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);

      lp->wx = -(lp->ly); // lx,ly を６４だけ回転
      lp->wy = lp->lx;    //

      lp->infx = cosl(lp->d, 800);
      lp->infy = sinl(lp->d, 800);

      SetLongPoint(lp); // p[4] をセット
    }
  }
}

void LaserManager::RotateLongRel(const EnemyData *e, char d, uint8_t id) {
  for (auto &it : long_lasers) {
    auto *lp = &it;
    if ((lp->e == e) && (lp->EnemyID == id || id == ECLCST_LLASERALL)) {
      lp->d += d;

      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);

      lp->wx = -(lp->ly); // lx,ly を６４だけ回転
      lp->wy = lp->lx;    //

      lp->infx = cosl(lp->d, 800);
      lp->infy = sinl(lp->d, 800);

      SetLongPoint(lp); // p[4] をセット
    }
  }
}

// 敵に関連づけられたレーザーを強制クローズ(Level2...) //
void LaserManager::ForceCloseLong(const EnemyData *e) {
  for (auto &it : long_lasers) {
    auto *lp = &it;

    // (参考) すでにクローズ状態であっても LLaserClose() は問題を発生させない //
    if (lp->e == e) {
      lp->flag = LLF_CLOSE;
      Snd_SEStop(2);
      // LLaserClose(i);
    }
  }
}

void LaserManager::MoveLong() {
  int i;
  LongLaserData *lp;

  for (i = 0, lp = long_lasers.data(); i < LLASER_MAX; i++, lp++) {

    // 角度セットモードで、敵の角度と現在の角度が異なっていたら再度セット //
    if (lp->type == LLS_SETDEG && lp->e && lp->d != lp->e->d) {
      lp->d = lp->e->d;

      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);

      lp->wx = -(lp->ly); // lx,ly を６４だけ回転
      lp->wy = lp->lx;    //

      lp->infx = cosl(lp->d, 800);
      lp->infy = sinl(lp->d, 800);

      SetLongPoint(lp); // p[4] をセット
    }

    switch (lp->flag) {
    // 太くなる場合 //
    case (LLF_OPEN):
      UpdateLongXY(i);
      lp->w += lp->v;

      if ((lp->w) >= (lp->wmax)) {
        lp->w = lp->wmax;
        lp->flag = LLF_NORM;
      }

      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;

      SetLongPoint(lp); // p[4] をセット
      HitCheckLong(lp);
      break;

    // 細くなる場合 //
    case (LLF_CLOSE):
    case (LLF_CLOSEL):
      UpdateLongXY(i);
      lp->w -= lp->v;

      if ((lp->w) <= 0) {
        lp->w = 0;
        if (lp->flag == LLF_CLOSE) {
          lp->flag = LLF_DISABLE;
          lp->e = nullptr;
        } else
          lp->flag = LLF_LINE;
      }

      lp->lx = cosl(lp->d, lp->w >> 6);
      lp->ly = sinl(lp->d, lp->w >> 6);
      lp->wx = -(lp->ly);
      lp->wy = lp->lx;

      SetLongPoint(lp); // p[4] をセット
                           //HitCheckLong(lp);
      break;

    // 直線状態 //
    case (LLF_LINE):
      UpdateLongXY(i);
      // この部分にレーザー溜めエフェクトを仕掛ける //
      // fragment_set(lp->x,lp->y,FRG_LASER);
      break;

    // ノーマル //
    case (LLF_NORM):
      UpdateLongXY(i);
      HitCheckLong(lp);
      break;

    case (LLF_DISABLE):
      break;
    }
  }
}

void LaserManager::DrawLong() {
  int x, y;
  VERTEX_XY p[4];
  int wx, wy, len;

  static const RGB216 Table16Bit[16] = {
      {3, 0, 3}, {0, 2, 0}, {0, 0, 4}, {4, 2, 0}, {0, 0, 1}};

  static const RGB216 Table8BitA[16] = {
      {2, 0, 2}, {0, 2, 0}, {0, 1, 3}, {4, 2, 0}, {0, 0, 1}};
  static const RGB216 Table8BitB[16] = {
      {3, 0, 3}, {0, 4, 0}, {0, 1, 5}, {5, 3, 0}, {2, 2, 4}};
  static const RGB216 Table8BitC[16] = {
      {5, 4, 5}, {5, 5, 5}, {4, 4, 5}, {5, 5, 4}, {4, 4, 5}};

  constexpr size_t VERTEX_COUNT = 34;
  std::array<VERTEX_XY, VERTEX_COUNT> p2;

  GrpGeom->Lock();

  for (const auto &it : long_lasers) {
    const auto *lp = &it;
    const auto c = lp->c;
    switch (lp->flag) {
    // 太さを持った状態 //
    case (LLF_OPEN):
    case (LLF_NORM):
    case (LLF_CLOSE):
    case (LLF_CLOSEL):
      x = ((lp->x) >> 6) + lp->lx;
      y = ((lp->y) >> 6) + lp->ly;
      wx = lp->wx;
      wy = lp->wy;
      len = isqrt(wx * wx + wy * wy);

      if (len) {
        /*
        p[0].x = p[1].x = lp->p[0].x ;//- wx*4/len;
        p[0].y = p[1].y = lp->p[0].y ;//- wy*4/len;
        p[3].x = p[2].x = lp->p[3].x ;//+ wx*4/len;
        p[3].y = p[2].y = lp->p[3].y ;//+ wy*4/len;
        p[1].x += lp->infx;
        p[1].y += lp->infy;
        p[2].x += lp->infx;
        p[2].y += lp->infy;
        */
        if (auto *gp = GrpGeom_Poly()) {
          // gp->SetColor({ 3, 0, 3 });
          const RGBA col = Table16Bit[c].ToRGB().WithAlpha(0xFF);
          gp->SetAlphaOne();
          GeomGrdRectA(*gp, lp->p, col);

          std::array<VERTEX_RGBA, VERTEX_COUNT> vcs;
          vcs[0] = {255, 255, 255, 0xFF};
          for (auto &vc : vcs | std::views::drop(1)) {
            vc = col;
          }

          p2[0].x = x;
          p2[0].y = y;
          p2[1].x = lp->p[0].x;
          p2[1].y = lp->p[0].y;
          p2[VERTEX_COUNT - 1].x = lp->p[3].x;
          p2[VERTEX_COUNT - 1].y = lp->p[3].y;
          for (auto n = 2; n < (VERTEX_COUNT - 1); n++) {
            p2[n].x = p2[0].x + cosl(lp->d + 64 + 128 * (n - 1) / 32, len);
            p2[n].y = p2[0].y + sinl(lp->d + 64 + 128 * (n - 1) / 32, len);
          }
          gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p2, vcs);
          break;
        } else if (auto *gf = GrpGeom_FB()) {
          // gf->SetColor({ 2, 0, 2 });
          gf->SetColor(Table8BitA[c]);
          gf->DrawTriangleFan(lp->p);
        }
      } else if (GrpGeom_Poly()) {
        break;
      }

      GeomCircleF({x, y}, len); // (lp->w >> 6) + 4);

      // GrpGeom->SetColor({ 3, 0, 3 }); // lp->c;
      GrpGeom->SetColor(Table8BitB[c]);
      if (len) {
        p[0].x = p[1].x = lp->p[0].x - wx / 8; //+ wx*2/len;
        p[0].y = p[1].y = lp->p[0].y - wy / 8; //+ wy*2/len;
        p[3].x = p[2].x = lp->p[3].x + wx / 8; //- wx*2/len;
        p[3].y = p[2].y = lp->p[3].y + wy / 8; //- wy*2/len;
        p[1].x += lp->infx;
        p[1].y += lp->infy;
        p[2].x += lp->infx;
        p[2].y += lp->infy;
        GrpGeom->DrawTriangleFan(p);
      }
      GeomCircleF({x, y}, (len - len / 8)); // (lp->w >> 6) + 2);

      // GrpGeom->SetColor({ 5, 4, 5 }); // lp->c;
      GrpGeom->SetColor(Table8BitC[c]);
      if (len) {
        p[0].x = p[1].x = lp->p[0].x - wx / 4; //+ wx*2/len;
        p[0].y = p[1].y = lp->p[0].y - wy / 4; //+ wy*2/len;
        p[3].x = p[2].x = lp->p[3].x + wx / 4; //- wx*2/len;
        p[3].y = p[2].y = lp->p[3].y + wy / 4; //- wy*2/len;
        p[1].x += lp->infx;
        p[1].y += lp->infy;
        p[2].x += lp->infx;
        p[2].y += lp->infy;
        GrpGeom->DrawTriangleFan(p);
      }
      GeomCircleF({x, y}, (len - len / 4)); // (lp->w >> 6));
      break;

    // ライン状態の場合 //
    case (LLF_LINE):
      x = (lp->x) >> 6;
      y = (lp->y) >> 6;
      GrpGeom->SetColor({4, 4, 4});
      GrpGeom->DrawLine(x, y, (x + lp->infx), (y + lp->infy));
      break;

    // 使用中で無い場合 //
    case (LLF_DISABLE):
      break;
    }
  }

  GrpGeom->Unlock();
}

void LaserManager::ClearLong() {
  // 存在するレーザー全てを閉じる //
  for (auto &it : long_lasers) {
    if (it.flag != LLF_DISABLE) {
      it.flag = LLF_CLOSE;
    }
  }

  Snd_SEStop(2);
}

void LaserManager::SetupLong() {
  for (auto &it : long_lasers) {
    // memset(long_lasers+i,0,sizeof(LLASER_DATA));
    it.flag = LLF_DISABLE;
    it.e = nullptr;
  }

  Snd_SEStop(2);
}

void LaserManager::SetLongPoint(LongLaserData *lp) {
  auto *pp = lp->p;

  pp[1].x = pp[0].x = (lp->x >> 6) + lp->wx + lp->lx;
  pp[1].y = pp[0].y = (lp->y >> 6) + lp->wy + lp->ly;

  pp[2].x = pp[3].x = (lp->x >> 6) - lp->wx + lp->lx;
  pp[2].y = pp[3].y = (lp->y >> 6) - lp->wy + lp->ly;

  pp[1].x += lp->infx;
  pp[1].y += lp->infy;

  pp[2].x += lp->infx;
  pp[2].y += lp->infy;
}

void LaserManager::HitCheckLong(const LongLaserData *lp) {
  //	long tx,ty,w1,w2,length;

  int tx, ty;
  int length, width;

  if (Players.viv.muteki)
    return;

  tx = Players.viv.x - lp->x;
  ty = Players.viv.y - lp->y;

  length = cosl(lp->d, tx) + sinl(lp->d, ty);
  width = abs(-sinl(lp->d, tx) + cosl(lp->d, ty));

  /*
          // 計算上の注意 : 座標の計算にはx64をつかう //
          // sinm(),cosm()を使っているので/256補正が必要となる //
          tx = ((lp->x)-(Players.viv.x));	ty = ((lp->y)-(Players.viv.y));
          length = -((cosm(lp->d)*tx+sinm(lp->d)*ty)>>8);
          tx <<= 8;	ty <<= 8;

          if(cosm(lp->d)==0)
                  w1 = abs((length*cosm(lp->d)+tx)/(-sinm(lp->d)));
          else if(sinm(lp->d)==0)
                  w1 = abs((length*sinm(lp->d)+ty)/( cosm(lp->d)));
          else{
                  w2 = abs((length*cosm(lp->d)+tx)/(-sinm(lp->d)));
                  w1 = abs((length*sinm(lp->d)+ty)/( cosm(lp->d)));
                  w1 = (w1+w2)/2;	// 精度アップ
          }
  */
  if (length > 0 && width <= (lp->w + 64 * 15)) {
    evade_add(LLASER_EVADE);
  }
  if (length > 0 && width <= (lp->w)) {
    MaidDead();
  }
}
