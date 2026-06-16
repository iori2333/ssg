/*
 *   Ending.cpp   : エンディングの処理
 *
 */

#include "ENDING.h"
#include "ending_manager.h"
#include "GIAN.h"
#include "SCL.h" // ＳＣＬ定義ファイル
#include "game/bgm.h"
#include "game/cast.h"
#include "game/endian.h"
#include "platform/text_backend.h"

// ファイル静的変数 → ending_manager.h の EndingManager に移動

void EndingManager::SetFixedColors(PALETTE &pal) {
  pal[255] = {0x00, 0x00, 0x00};
  pal[199] = {0xFF, 0xFF, 0xFF};
  pal[198] = {0x80, 0x80, 0x80};
}

// エンディングまわりの初期化 //
bool EndingManager::Init() {
  PALETTE pal;

  GrpBackend_SetClip(GRP_RES_RECT);
  GrpBackend_Clear();
  Grp_Flip();
  GrpBackend_Clear();

  if (!LoadGraph(GRAPH_ID_ENDING) || !LoadStageData(GRAPH_ID_ENDING)) {
    return false;
  }
  BGM_Stop();

  GrpBackend_PaletteGet(pal);
  this->SetFixedColors(pal);
  GrpBackend_PaletteSet(pal);

  GameMain = EndingProc;

  this->flash_state = 0;

  this->grp_info.bWantDisp = false;
  this->stf_task.bWantDisp = false;

  TextObj.Clear();
  this->text.Blank();
  this->text.Rect = TextObj.Register({GRP_RES.w, 131});

  return true;
}

void EndingManager::Proc(bool &) {
  extern bool IsDraw();

  if (this->flash_state)
    this->flash_state -= 32;

  this->SCLDecode();
  if (!GameMainIs(EndingProc))
    return;

  if (IsDraw()) {
    this->UpdateGrpInfo();
    this->UpdateStfInfo();
    this->Draw();
  }
}

// エンディング時の描画処理 //
void EndingManager::Draw() {
  // 画面消去 //
  GrpBackend_Clear(255, RGB{0, 0, 0});

  // それぞれのグラフィックを描画するで //
  this->DrawGrpInfo();
  this->DrawStfInfo();
  this->text.Render({0, 349});

  // フェード情報の反映ぢゃ //
  this->DrawFadeInfo();

  Grp_Flip();
}

// グラフィックのフェードアウト用関数 //
void EndingManager::FadeoutPaletteGrp(PALETTE &Dest, const PALETTE &Src, uint8_t a) {
  Dest = Src.Fade(a, 0, 199);
  EndingManager::SetFixedColors(Dest);
}

// スタッフ名のフェードアウト用関数 //
void EndingManager::FadeoutPaletteStf(PALETTE &Dest, const PALETTE &Src, uint8_t a) {
  Dest = Src.Fade(a, 200, 255);
  EndingManager::SetFixedColors(Dest);
}

// グラフィックの更新(内部データ) //
void EndingManager::UpdateGrpInfo() {
  this->grp_info.timer++;
  if (this->grp_info.timer > this->grp_info.fadeout) {
    if (this->grp_info.alpha - 3 > 0)
      this->grp_info.alpha -= 3;
    else
      this->grp_info.alpha = 0;
  } else if (this->grp_info.timer > this->grp_info.fadein) {
    if (this->grp_info.alpha + 3 < 255)
      this->grp_info.alpha += 3;
    else
      this->grp_info.alpha = 255;
  }

  if (this->grp_info.bWantDisp && this->grp_info.alpha == 0)
    this->grp_info.bWantDisp = false;
}

// スタッフの更新(内部データ)
void EndingManager::UpdateStfInfo() {
  this->stf_task.timer++;
  if (this->stf_task.timer > this->stf_task.fadeout) {
    if (this->stf_task.alpha - 3 > 0)
      this->stf_task.alpha -= 3;
    else
      this->stf_task.alpha = 0;
  } else if (this->stf_task.timer > this->stf_task.fadein) {
    if (this->stf_task.alpha + 3 < 255)
      this->stf_task.alpha += 3;
    else
      this->stf_task.alpha = 255;
  }

  if (this->stf_task.bWantDisp && this->stf_task.alpha == 0)
    this->stf_task.bWantDisp = false;
}

// グラフィックの描画 //
void EndingManager::DrawGrpInfo() {
  if (!this->grp_info.bWantDisp)
    return;

  // 驚異の画像表示 //
  const auto sid = (SURFACE_ID::ENDING_PIC + (this->grp_info.target - EndingGrp));
  GrpSurface_BlitOpaque({this->grp_info.x, this->grp_info.y}, sid, {0, 0, 320, 240});
}

// スタッフの描画 //
void EndingManager::DrawStfInfo() {
  if (!this->stf_task.bWantDisp)
    return;

  auto Blit = [](WINDOW_POINT dst, const PIXEL_LTRB &src) {
    dst -= (src.Size() / 2);
    GrpSurface_Blit({dst.x, dst.y}, SURFACE_ID::ENDING_CREDITS, src);
  };

  Blit({this->stf_task.ox, this->stf_task.oy}, staff_label[this->stf_task.TitleID]);
  for (decltype(this->stf_task.NumStf) i = 0; i < this->stf_task.NumStf; i++) {
    const WINDOW_POINT dst = {this->stf_task.ox, (this->stf_task.oy + (i * 30) + 50)};
    Blit(dst, staff_member[this->stf_task.StfID[i]]);
  }
}

// テキストの描画 //
void EndingManager::Text::Render(WINDOW_POINT topleft) {
  TextObj.Render(topleft, Rect, TextStr, [this](TEXTRENDER_SESSION &s) {
    int max = 0;

    for (decltype(NumText) i = 0; i < NumText; i++) {
      max = (std::max)(max, static_cast<int>(Text[i].size()));
    }

    const auto dx = (8 * (39 - (max / 2)));

    s.SetFont(FONT_ID::NORMAL);
    s.SetColor({128, 128, 128});
    for (decltype(NumText) i = 0; i < NumText; i++) {
      s.Put({(dx + 21), (1 + (i * 25))}, Text[i]);
      s.Put({(dx + 19), (1 + (i * 25))}, Text[i]);
      s.Put({(dx + 20), (0 + (i * 25))}, Text[i]);
      s.Put({(dx + 20), (2 + (i * 25))}, Text[i]);
    }

    s.SetColor({255, 255, 255});
    for (decltype(NumText) i = 0; i < NumText; i++) {
      s.Put({(dx + 20), (1 + (i * 25))}, Text[i]);
    }
  });
}

void EndingManager::FlashPaletteGrp(PALETTE &dest, const PALETTE &pal, uint16_t a) {
  const uint16_t a16 = ((a > 256) ? (a - 256) : a);
  for (int i = 0; i < dest.size(); i++) {
    dest[i].r = (std::min)(256, (256 * (256 - a) + (pal[i].r * a16)) / 256);
    dest[i].g = (std::min)(256, (256 * (256 - a) + (pal[i].g * a16)) / 256);
    dest[i].b = (std::min)(256, (256 * (256 - a) + (pal[i].b * a16)) / 256);
  }
}

// フェードＩＯ情報の反映 //
void EndingManager::DrawFadeInfo() {
  PALETTE temp_pal;

  // フェードアウト関連
  if (GrpGeom_FB()) {
    if (this->flash_state) {
      this->FlashPaletteGrp(temp_pal, this->grp_info.target->pal, this->flash_state);
      GrpBackend_PaletteSet(temp_pal);
    } else if (this->grp_info.target) {
      this->FadeoutPaletteGrp(temp_pal, this->grp_info.target->pal,
                        Cast::down_sign<uint8_t>(this->grp_info.alpha));
      this->FadeoutPaletteStf(temp_pal, temp_pal,
                        Cast::down_sign<uint8_t>(this->stf_task.alpha));
      GrpBackend_PaletteSet(temp_pal);
    } else {
      temp_pal = {0};
      this->SetFixedColors(temp_pal);
      GrpBackend_PaletteSet(temp_pal);
    }
  } else {
    GrpGeom->Lock();

    if (this->grp_info.bWantDisp) {
      GrpGeom->SetAlphaNorm(255 - this->grp_info.alpha);
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->DrawBoxA(this->grp_info.x, this->grp_info.y, (this->grp_info.x + 320),
                        (this->grp_info.y + 240));
    }
    if (this->stf_task.bWantDisp) {
      GrpGeom->SetAlphaNorm(255 - this->stf_task.alpha);
      GrpGeom->SetColor({0, 0, 0});
      if (this->stf_task.ox == 320) {
        GrpGeom->DrawBoxA(0, 0, GRP_RES.w, GRP_RES.h);
      } else if (this->stf_task.ox > 320) {
        GrpGeom->DrawBoxA(320, 0, GRP_RES.w, 300);
      } else {
        GrpGeom->DrawBoxA(0, 0, (320 - 50), 300);
      }
    }
    if (this->flash_state) {
      GrpGeom->SetAlphaNorm(255 - this->flash_state);
      GrpGeom->SetColor({5, 5, 5});
      GrpGeom->DrawBoxA(0, 0, GRP_RES.w, GRP_RES.h);
    }

    GrpGeom->Unlock();
  }
}

// エンディング用 SCL のデコード //
void EndingManager::SCLDecode() {
  bool bFlag = true;

  while (bFlag) {
    const uint8_t *cmd = SCL_Now;
    switch (cmd[0]) {
    case (SCL_TIME): {
      const auto temp = I32LEAt(&cmd[1]);
      if (temp > GameCount) {
        bFlag = false;
      } else {
        SCL_Now += 5; // cmd(1)+time(4)
      }
      break;
    }

    case (SCL_MSG): { // メッセージを出力する
      const auto *line_p = std::bit_cast<const char *>(cmd + 1);
      const Narrow::string_view line = line_p;
      this->text.Text[this->text.NumText++] = line;
      this->text.TextStr += line_p;
      this->text.TextStr += '\n';
      SCL_Now += (line.length() + 2);
      break;
    }

    case (SCL_FACE): // 顔を表示する
      switch (cmd[1]) {
      case 0:
        this->grp_info.fadein = 0;
        this->grp_info.fadeout = 128 + 64 + 64 + 512;
        this->grp_info.x = 640 - 40 - 320;
        this->grp_info.y = 40;
        break;

      case 1:
      case 2:
      case 3:
        this->grp_info.fadein = 0;
        this->grp_info.fadeout = 128 + 64;
        this->grp_info.x = 320 - 160;
        this->grp_info.y = 40;
        break;

      case 5:
        this->grp_info.fadein = 0;
        this->grp_info.fadeout = 128 + 64 + 64 + (512 + 512) * 2;
        this->grp_info.x = 40;
        this->grp_info.y = 40;
        break;

      default:
        this->grp_info.fadein = 0;
        this->grp_info.fadeout = 128 + 64 + 64 + 512;
        this->grp_info.x = 40;
        this->grp_info.y = 40;
        break;
      }
      this->grp_info.alpha = 0;
      this->grp_info.target = EndingGrp + cmd[1];
      this->grp_info.timer = 0;
      this->grp_info.bWantDisp = true;
      SCL_Now += 2;
      break;

    case (SCL_STAFF): // わかりにくいが、１２８を加えると、役割名指定ね
      if (cmd[1] >= 128) {
        switch (cmd[1] - 128) {
        case 0:
        case 4:
          this->stf_task.fadein = 0;
          this->stf_task.fadeout = 128 + 64 + 64 + 128;
          this->stf_task.ox = 320 + 130;
          this->stf_task.oy = 80 + 50;
          break;
        case 2:
        case 5:
          this->stf_task.fadein = 0;
          this->stf_task.fadeout = 128 + 64 + 64 + 128;
          this->stf_task.ox = 320 + 130;
          this->stf_task.oy = 80;
          break;
        case 1:
        case 3:
          this->stf_task.fadein = 0;
          this->stf_task.fadeout = 128 + 64 + 64 + 128;
          this->stf_task.ox = 130;
          this->stf_task.oy = 80 + 50;
          break;
        case 6:
          this->stf_task.fadein = 0;
          this->stf_task.fadeout = 128 + 64 + 64; //+64;
          this->stf_task.ox = 320;
          this->stf_task.oy = 80 + 80;
          break;
        }
        this->stf_task.alpha = 0;
        this->stf_task.timer = 0;
        this->stf_task.timer = 0;
        this->stf_task.NumStf = 0;
        this->stf_task.TitleID = cmd[1] - 128;
        this->stf_task.bWantDisp = true;
      } else {
        this->stf_task.StfID[this->stf_task.NumStf++] = cmd[1];
      }
      SCL_Now += 2;
      break;

    case (SCL_NPG): // 新しいページに変更する
      this->text.Blank();
      SCL_Now++;
      break;

    case (SCL_END): // カウントも変更させずにリターンするのだ
      this->grp_info.bWantDisp = false;
      this->stf_task.bWantDisp = false;
      NameRegistInit(false);
      return;

    case (SCL_MUSIC):
      BGM_Switch(cmd[1]);
      SCL_Now += 2;
      break;

    case (SCL_EFC):
      switch (cmd[1]) {
      case 0:
        this->flash_state = 256 * 2;
        break;
      }

      SCL_Now += 2;
      break;

    case (SCL_STAGECLEAR): // ステージクリア
      return;

    case (SCL_GAMECLEAR):
      return;

    default: // 未実装 or ばぐ
      return;
    }
  }

  GameCount++;
}
