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
  SetFixedColors(pal);
  GrpBackend_PaletteSet(pal);

  GameFlow.game_main = EndingProc;
  GameFlow.current_state = GameState::Ending;

  flash_state = 0;

  grp_info.bWantDisp = false;
  stf_task.bWantDisp = false;

  TextObj.Clear();
  text.Blank();
  text.Rect = TextObj.Register({GRP_RES.w, 131});

  return true;
}

void EndingManager::Proc(bool &) {
  if (flash_state)
    flash_state -= 32;

  SCLDecode();
  if (GameFlow.current_state != GameState::Ending)
    return;

  if (GameFlow.IsDraw()) {
    UpdateGrpInfo();
    UpdateStfInfo();
    Draw();
  }
}

// エンディング時の描画処理 //
void EndingManager::Draw() {
  // 画面消去 //
  GrpBackend_Clear(255, RGB{0, 0, 0});

  // それぞれのグラフィックを描画するで //
  DrawGrpInfo();
  DrawStfInfo();
  text.Render({0, 349});

  // フェード情報の反映ぢゃ //
  DrawFadeInfo();

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
  grp_info.timer++;
  if (grp_info.timer > grp_info.fadeout) {
    if (grp_info.alpha - 3 > 0)
      grp_info.alpha -= 3;
    else
      grp_info.alpha = 0;
  } else if (grp_info.timer > grp_info.fadein) {
    if (grp_info.alpha + 3 < 255)
      grp_info.alpha += 3;
    else
      grp_info.alpha = 255;
  }

  if (grp_info.bWantDisp && grp_info.alpha == 0)
    grp_info.bWantDisp = false;
}

// スタッフの更新(内部データ)
void EndingManager::UpdateStfInfo() {
  stf_task.timer++;
  if (stf_task.timer > stf_task.fadeout) {
    if (stf_task.alpha - 3 > 0)
      stf_task.alpha -= 3;
    else
      stf_task.alpha = 0;
  } else if (stf_task.timer > stf_task.fadein) {
    if (stf_task.alpha + 3 < 255)
      stf_task.alpha += 3;
    else
      stf_task.alpha = 255;
  }

  if (stf_task.bWantDisp && stf_task.alpha == 0)
    stf_task.bWantDisp = false;
}

// グラフィックの描画 //
void EndingManager::DrawGrpInfo() {
  if (!grp_info.bWantDisp)
    return;

  // 驚異の画像表示 //
  const auto sid = (SURFACE_ID::ENDING_PIC + (grp_info.target - EndingGrp));
  GrpSurface_BlitOpaque({grp_info.x, grp_info.y}, sid, {0, 0, 320, 240});
}

// スタッフの描画 //
void EndingManager::DrawStfInfo() {
  if (!stf_task.bWantDisp)
    return;

  auto Blit = [](WINDOW_POINT dst, const PIXEL_LTRB &src) {
    dst -= (src.Size() / 2);
    GrpSurface_Blit({dst.x, dst.y}, SURFACE_ID::ENDING_CREDITS, src);
  };

  Blit({stf_task.ox, stf_task.oy}, staff_label[stf_task.TitleID]);
  for (decltype(stf_task.NumStf) i = 0; i < stf_task.NumStf; i++) {
    const WINDOW_POINT dst = {stf_task.ox, (stf_task.oy + (i * 30) + 50)};
    Blit(dst, staff_member[stf_task.StfID[i]]);
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
    if (flash_state) {
      FlashPaletteGrp(temp_pal, grp_info.target->pal, flash_state);
      GrpBackend_PaletteSet(temp_pal);
    } else if (grp_info.target) {
      FadeoutPaletteGrp(temp_pal, grp_info.target->pal,
                        Cast::down_sign<uint8_t>(grp_info.alpha));
      FadeoutPaletteStf(temp_pal, temp_pal,
                        Cast::down_sign<uint8_t>(stf_task.alpha));
      GrpBackend_PaletteSet(temp_pal);
    } else {
      temp_pal = {0};
      SetFixedColors(temp_pal);
      GrpBackend_PaletteSet(temp_pal);
    }
  } else {
    GrpGeom->Lock();

    if (grp_info.bWantDisp) {
      GrpGeom->SetAlphaNorm(255 - grp_info.alpha);
      GrpGeom->SetColor({0, 0, 0});
      GrpGeom->DrawBoxA(grp_info.x, grp_info.y, (grp_info.x + 320),
                        (grp_info.y + 240));
    }
    if (stf_task.bWantDisp) {
      GrpGeom->SetAlphaNorm(255 - stf_task.alpha);
      GrpGeom->SetColor({0, 0, 0});
      if (stf_task.ox == 320) {
        GrpGeom->DrawBoxA(0, 0, GRP_RES.w, GRP_RES.h);
      } else if (stf_task.ox > 320) {
        GrpGeom->DrawBoxA(320, 0, GRP_RES.w, 300);
      } else {
        GrpGeom->DrawBoxA(0, 0, (320 - 50), 300);
      }
    }
    if (flash_state) {
      GrpGeom->SetAlphaNorm(255 - flash_state);
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
    const uint8_t *cmd = Enemies.scl_now;
    switch (cmd[0]) {
    case (SCL_TIME): {
      const auto temp = I32LEAt(&cmd[1]);
      if (temp > GameState.game_count) {
        bFlag = false;
      } else {
        Enemies.scl_now += 5; // cmd(1)+time(4)
      }
      break;
    }

    case (SCL_MSG): { // メッセージを出力する
      const auto *line_p = std::bit_cast<const char *>(cmd + 1);
      const Narrow::string_view line = line_p;
      text.Text[text.NumText++] = line;
      text.TextStr += line_p;
      text.TextStr += '\n';
      Enemies.scl_now += (line.length() + 2);
      break;
    }

    case (SCL_FACE): // 顔を表示する
      switch (cmd[1]) {
      case 0:
        grp_info.fadein = 0;
        grp_info.fadeout = 128 + 64 + 64 + 512;
        grp_info.x = 640 - 40 - 320;
        grp_info.y = 40;
        break;

      case 1:
      case 2:
      case 3:
        grp_info.fadein = 0;
        grp_info.fadeout = 128 + 64;
        grp_info.x = 320 - 160;
        grp_info.y = 40;
        break;

      case 5:
        grp_info.fadein = 0;
        grp_info.fadeout = 128 + 64 + 64 + (512 + 512) * 2;
        grp_info.x = 40;
        grp_info.y = 40;
        break;

      default:
        grp_info.fadein = 0;
        grp_info.fadeout = 128 + 64 + 64 + 512;
        grp_info.x = 40;
        grp_info.y = 40;
        break;
      }
      grp_info.alpha = 0;
      grp_info.target = EndingGrp + cmd[1];
      grp_info.timer = 0;
      grp_info.bWantDisp = true;
      Enemies.scl_now += 2;
      break;

    case (SCL_STAFF): // わかりにくいが、１２８を加えると、役割名指定ね
      if (cmd[1] >= 128) {
        switch (cmd[1] - 128) {
        case 0:
        case 4:
          stf_task.fadein = 0;
          stf_task.fadeout = 128 + 64 + 64 + 128;
          stf_task.ox = 320 + 130;
          stf_task.oy = 80 + 50;
          break;
        case 2:
        case 5:
          stf_task.fadein = 0;
          stf_task.fadeout = 128 + 64 + 64 + 128;
          stf_task.ox = 320 + 130;
          stf_task.oy = 80;
          break;
        case 1:
        case 3:
          stf_task.fadein = 0;
          stf_task.fadeout = 128 + 64 + 64 + 128;
          stf_task.ox = 130;
          stf_task.oy = 80 + 50;
          break;
        case 6:
          stf_task.fadein = 0;
          stf_task.fadeout = 128 + 64 + 64; //+64;
          stf_task.ox = 320;
          stf_task.oy = 80 + 80;
          break;
        }
        stf_task.alpha = 0;
        stf_task.timer = 0;
        stf_task.timer = 0;
        stf_task.NumStf = 0;
        stf_task.TitleID = cmd[1] - 128;
        stf_task.bWantDisp = true;
      } else {
        stf_task.StfID[stf_task.NumStf++] = cmd[1];
      }
      Enemies.scl_now += 2;
      break;

    case (SCL_NPG): // 新しいページに変更する
      text.Blank();
      Enemies.scl_now++;
      break;

    case (SCL_END): // カウントも変更させずにリターンするのだ
      grp_info.bWantDisp = false;
      stf_task.bWantDisp = false;
      GameFlow.NameRegistInit(false);
      return;

    case (SCL_MUSIC):
      BGM_Switch(cmd[1]);
      Enemies.scl_now += 2;
      break;

    case (SCL_EFC):
      switch (cmd[1]) {
      case 0:
        flash_state = 256 * 2;
        break;
      }

      Enemies.scl_now += 2;
      break;

    case (SCL_STAGECLEAR): // ステージクリア
      return;

    case (SCL_GAMECLEAR):
      return;

    default: // 未実装 or ばぐ
      return;
    }
  }

  GameState.game_count++;
}
