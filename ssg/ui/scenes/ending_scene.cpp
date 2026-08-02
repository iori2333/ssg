/// Ending cinematic UI scene.

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "ending_scene.h"

#include "audio/audio_system.h"
#include "data/graphics_assets.h"
#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/text_ttf.h"
#include "i18n/localization.h"
#include "music/music_player.h"
#include "stage/scene_program.h"
#include "stage/stage_loader.h"

// Ending initialization

bool EndingScene::Enter() {
  GraphicsBackendSetClip(kGameResolutionRect);
  GraphicsBackendClear();
  GraphicsFlip();
  GraphicsBackendClear();

  if (!graphics_.LoadEnding() || !stage::StageLoader::LoadEnding(scene_)) {
    return false;
  }
  audio_.StopBgm();

  flash_state = 0;

  grp_info.bWantDisp = false;
  stf_task.bWantDisp = false;

  TextRenderer().Clear();
  text.Blank();
  text.Rect = TextRenderer().Register({.w = kGameResolution.w, .h = 131});

  return true;
}

bool EndingScene::Update(bool should_draw) {
  if (flash_state > 0) {
    flash_state = std::max(0, flash_state - 32);
  }

  if (SCLDecode()) {
    return true;
  }

  if (should_draw) {
    UpdateGrpInfo();
    UpdateStfInfo();
    Draw();
  }
  return false;
}

// Ending draw processing
void EndingScene::Draw() {
  // Clear screen
  GraphicsBackendClear(255, Rgb{.r = 0, .g = 0, .b = 0});

  // Draw each graphic
  DrawGrpInfo();
  DrawStfInfo();
  text.Render({0, 349});

  // Apply fade info
  DrawFadeInfo();

  GraphicsFlip();
}

// Staff name fadeout function
void EndingScene::UpdateGrpInfo() {
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
void EndingScene::UpdateStfInfo() {
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
void EndingScene::DrawGrpInfo() {
  if (!grp_info.bWantDisp) {
    return;
  }

  // Display image
  const auto sid =
      data::graphics_assets::EndingPictureSurface(grp_info.picture_id);
  GraphicsSurfaceBlitOpaque({grp_info.x, grp_info.y}, sid, {0, 0, 320, 240});
}

// Staff drawing
void EndingScene::DrawStfInfo() {
  if (!stf_task.bWantDisp) {
    return;
  }

  auto Blit = [](WindowPoint dst, const PixelLtrb &src) {
    dst -= (src.Size() / 2);
    GraphicsSurfaceBlit({dst.x, dst.y}, SurfaceId::EndingCredits, src);
  };

  Blit({stf_task.ox, stf_task.oy}, staff_label[stf_task.TitleID]);
  for (std::size_t i = 0; i < stf_task.NumStf; i++) {
    const WindowPoint dst = {
        stf_task.ox,
        static_cast<int>(stf_task.oy + (i * 30) + 50),
    };
    Blit(dst, staff_member[stf_task.StfID[i]]);
  }
}

// Text drawing
void EndingScene::Text::Render(WindowPoint topleft) {
  TextRenderer().Render(topleft, Rect, TextStr, [this](TextRenderSession &s) {
    int max_px = 0;

    s.SetFont(FontId::Normal);
    for (auto i : Text) {
      max_px = (std::max)(max_px, s.Extent(i).w);
    }

    const auto dx = std::max(0, (s.RectSize().w - max_px) / 2);

    s.SetColor({.r = 128, .g = 128, .b = 128});
    for (size_t i = 0; i < Text.size(); i++) {
      const auto y = static_cast<int>(i) * 25;
      s.Put({.x = (dx + 1), .y = (y + 1)}, Text[i]);
      s.Put({.x = (dx - 1), .y = (y + 1)}, Text[i]);
      s.Put({.x = dx, .y = y}, Text[i]);
      s.Put({.x = dx, .y = (y + 2)}, Text[i]);
    }

    s.SetColor({.r = 255, .g = 255, .b = 255});
    for (size_t i = 0; i < Text.size(); i++) {
      const auto y = static_cast<int>(i) * 25;
      s.Put({.x = dx, .y = (y + 1)}, Text[i]);
    }
  });
}

// Apply fade I/O info
void EndingScene::DrawFadeInfo() const {

  if (grp_info.bWantDisp) {
    geometry::SetAlphaNorm(255 - grp_info.alpha);
    geometry::SetColor({0, 0, 0});
    geometry::DrawBoxA(grp_info.x, grp_info.y, (grp_info.x + 320),
                       (grp_info.y + 240));
  }
  if (stf_task.bWantDisp) {
    geometry::SetAlphaNorm(255 - stf_task.alpha);
    geometry::SetColor({0, 0, 0});
    if (stf_task.ox == 320) {
      geometry::DrawBoxA(0, 0, kGameResolution.w, kGameResolution.h);
    } else if (stf_task.ox > 320) {
      geometry::DrawBoxA(320, 0, kGameResolution.w, 300);
    } else {
      geometry::DrawBoxA(0, 0, (320 - 50), 300);
    }
  }
  if (flash_state > 0) {
    geometry::SetAlphaNorm(255 - flash_state);
    geometry::SetColor({5, 5, 5});
    geometry::DrawBoxA(0, 0, kGameResolution.w, kGameResolution.h);
  }
}

// Ending SCL decode
bool EndingScene::SCLDecode() {
  while (const auto *instruction = scene_.Current()) {
    switch (instruction->opcode) {
    case stage::SceneOpcode::Time:
      if (instruction->value > scene_.Frame()) {
        scene_.AdvanceFrame();
        return false;
      }
      scene_.Advance();
      break;

    case stage::SceneOpcode::Message:
      text.Text.push_back(instruction->text);
      text.TextStr += instruction->text;
      text.TextStr += '\n';
      scene_.Advance();
      break;

    case stage::SceneOpcode::MessageReference:
      for (const auto line : localization_.Lines(instruction->text_id)) {
        text.Text.push_back(line);
        text.TextStr += line;
        text.TextStr += '\n';
      }
      scene_.Advance();
      break;

    case stage::SceneOpcode::Face:
      switch (instruction->face_id) {
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
      grp_info.picture_id = instruction->face_id;
      grp_info.timer = 0;
      grp_info.bWantDisp = true;
      scene_.Advance();
      break;

    case stage::SceneOpcode::Staff: // Adding 128 specifies a role name
      if (instruction->staff_id >= 128) {
        switch (instruction->staff_id - 128) {
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
        default:
          break;
        }
        stf_task.alpha = 0;
        stf_task.timer = 0;
        stf_task.timer = 0;
        stf_task.NumStf = 0;
        stf_task.TitleID = instruction->staff_id - 128;
        stf_task.bWantDisp = true;
      } else {
        stf_task.StfID[stf_task.NumStf++] = instruction->staff_id;
      }
      scene_.Advance();
      break;

    case stage::SceneOpcode::NewPage:
      text.Blank();
      scene_.Advance();
      break;

    case stage::SceneOpcode::End:
      grp_info.bWantDisp = false;
      stf_task.bWantDisp = false;
      return true;

    case stage::SceneOpcode::Music:
      music_.Play(instruction->track_id);
      scene_.Advance();
      break;

    case stage::SceneOpcode::Effect:
      switch (instruction->effect) {
      case stage::SceneEffect::EndingFlash:
        flash_state = 256 * 2;
        break;
      default:
        break;
      }
      scene_.Advance();
      break;

    case stage::SceneOpcode::StageClear:
    case stage::SceneOpcode::GameClear:
    default:
      return false;
    }
  }
  return false;
}
