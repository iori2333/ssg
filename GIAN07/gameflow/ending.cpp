///
/// Ending - Ending processing
///

#include <utility>

#include "ending.h"
#include "ending_manager.h"

#include "audio/bgm.h"
#include "core/gian.h"
#include "enemy/enemy_manager.h"
#include "gameflow/gameflow_manager.h"
#include "platform/text_backend.h"
#include "stage/scene.h"
#include "stage/scroll_manager.h"
#include "util/cast.h"
#include "util/endian.h"

// File-static variables moved to EndingManager in ending_manager.h

// Ending initialization
bool EndingManager::Init() {
  GrpBackend_SetClip(GRP_RES_RECT);
  GrpBackend_Clear();
  Grp_Flip();
  GrpBackend_Clear();

  if (!GameFlow.ctx.graphics.LoadEnding() ||
      !GameFlow.ctx.stages.LoadEnding(Enemies, Scroller, GameFlow.ctx.game)) {
    return false;
  }
  BGM_Stop();

  GameFlow.game_main = [](bool &q) { GameFlow.ctx.ending.Proc(q); };
  GameFlow.current_state = GameState::Ending;

  flash_state = 0;

  grp_info.bWantDisp = false;
  stf_task.bWantDisp = false;

  TextObj.Clear();
  text.Blank();
  text.Rect = TextObj.Register({.w = GRP_RES.w, .h = 131});

  return true;
}

void EndingManager::Proc(bool & /*unused*/) {
  if (flash_state != 0U) {
    flash_state -= 32;
  }

  SCLDecode();
  if (GameFlow.current_state != GameState::Ending) {
    return;
  }

  if (GameFlow.IsDraw()) {
    UpdateGrpInfo();
    UpdateStfInfo();
    Draw();
  }
}

// Ending draw processing
void EndingManager::Draw() {
  // Clear screen
  GrpBackend_Clear(255, RGB{.r = 0, .g = 0, .b = 0});

  // Draw each graphic
  DrawGrpInfo();
  DrawStfInfo();
  text.Render({0, 349});

  // Apply fade info
  DrawFadeInfo();

  Grp_Flip();
}

// Staff name fadeout function
void EndingManager::UpdateGrpInfo() {
  grp_info.timer++;
  if (grp_info.timer > grp_info.fadeout) {
    if (grp_info.alpha - 3 > 0) {
      grp_info.alpha -= 3;
    } else {
      grp_info.alpha = 0;
    }
  } else if (grp_info.timer > grp_info.fadein) {
    if (grp_info.alpha + 3 < 255) {
      grp_info.alpha += 3;
    } else {
      grp_info.alpha = 255;
    }
  }

  if (grp_info.bWantDisp && grp_info.alpha == 0) {
    grp_info.bWantDisp = false;
  }
}

// Staff update (internal data)
void EndingManager::UpdateStfInfo() {
  stf_task.timer++;
  if (stf_task.timer > stf_task.fadeout) {
    if (stf_task.alpha - 3 > 0) {
      stf_task.alpha -= 3;
    } else {
      stf_task.alpha = 0;
    }
  } else if (stf_task.timer > stf_task.fadein) {
    if (stf_task.alpha + 3 < 255) {
      stf_task.alpha += 3;
    } else {
      stf_task.alpha = 255;
    }
  }

  if (stf_task.bWantDisp && stf_task.alpha == 0) {
    stf_task.bWantDisp = false;
  }
}

// Graphic drawing
void EndingManager::DrawGrpInfo() {
  if (!grp_info.bWantDisp) {
    return;
  }

  // Display image
  const auto sid = SURFACE_ID::ENDING_PIC + grp_info.picture_id;
  GrpSurface_BlitOpaque({grp_info.x, grp_info.y}, sid, {0, 0, 320, 240});
}

// Staff drawing
void EndingManager::DrawStfInfo() {
  if (!stf_task.bWantDisp) {
    return;
  }

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

// Text drawing
void EndingManager::Text::Render(WINDOW_POINT topleft) {
  TextObj.Render(topleft, Rect, TextStr, [this](TEXTRENDER_SESSION &s) {
    int max_px = 0;

    s.SetFont(FONT_ID::NORMAL);
    for (decltype(NumText) i = 0; i < NumText; i++) {
      max_px = (std::max)(max_px, s.Extent(Text[i]).w);
    }

    const auto dx = std::max(0, (s.RectSize().w - max_px) / 2);

    s.SetColor({.r = 128, .g = 128, .b = 128});
    for (decltype(NumText) i = 0; i < NumText; i++) {
      s.Put({.x = (dx + 1), .y = (1 + (i * 25))}, Text[i]);
      s.Put({.x = (dx - 1), .y = (1 + (i * 25))}, Text[i]);
      s.Put({.x = dx, .y = (0 + (i * 25))}, Text[i]);
      s.Put({.x = dx, .y = (2 + (i * 25))}, Text[i]);
    }

    s.SetColor({.r = 255, .g = 255, .b = 255});
    for (decltype(NumText) i = 0; i < NumText; i++) {
      s.Put({.x = dx, .y = (1 + (i * 25))}, Text[i]);
    }
  });
}

// Apply fade I/O info
void EndingManager::DrawFadeInfo() {
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
  if (flash_state != 0U) {
    GrpGeom->SetAlphaNorm(255 - flash_state);
    GrpGeom->SetColor({5, 5, 5});
    GrpGeom->DrawBoxA(0, 0, GRP_RES.w, GRP_RES.h);
  }

  GrpGeom->Unlock();
}

// Ending SCL decode
void EndingManager::SCLDecode() {
  bool bFlag = true;

  while (bFlag) {
    const uint8_t *cmd = Enemies.scl_now;
    switch (cmd[0]) {
    case SCL_TIME: {
      const auto temp = I32LEAt(&cmd[1]);
      if (std::cmp_greater(temp, GameFlow.ctx.game.count)) {
        bFlag = false;
      } else {
        Enemies.scl_now += 5; // cmd(1)+time(4)
      }
      break;
    }

    case SCL_MSG: { // Output message
      const auto *line_p = std::bit_cast<const char *>(cmd + 1);
      std::string_view line = line_p;
      text.Text[text.NumText++] = line;
      text.TextStr += line_p;
      text.TextStr += '\n';
      Enemies.scl_now += (line.length() + 2);
      break;
    }

    case SCL_FACE: // Display face
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
        grp_info.fadeout = 128 + 64 + 64 + ((512 + 512) * 2);
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
      grp_info.picture_id = cmd[1];
      grp_info.timer = 0;
      grp_info.bWantDisp = true;
      Enemies.scl_now += 2;
      break;

    case SCL_STAFF: // Adding 128 specifies a role name
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

    case SCL_NPG: // Switch to new page
      text.Blank();
      Enemies.scl_now++;
      break;

    case SCL_END: // Return without changing count
      grp_info.bWantDisp = false;
      stf_task.bWantDisp = false;
      GameFlow.NameRegistInit(false);
      return;

    case SCL_MUSIC:
      GameFlow.ctx.tracks.Switch(cmd[1]);
      Enemies.scl_now += 2;
      break;

    case SCL_EFC:
      switch (cmd[1]) {
      case 0:
        flash_state = 256 * 2;
        break;
      }

      Enemies.scl_now += 2;
      break;

    case SCL_STAGECLEAR: // Stage clear
      return;

    case SCL_GAMECLEAR:
      return;

    default: // Unimplemented or bug
      return;
    }
  }

  GameFlow.ctx.game.count++;
}
