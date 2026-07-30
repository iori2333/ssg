/// Debug bullet gallery scene.

#include <array>
#include <cstdint>
#include <format>
#include <string>

#include "bullet_gallery_scene.h"

#include "bullet/bullet_manager.h"
#include "data/graphics_loader.h"
#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/text.h"
#include "i18n/localization.h"
#include "platform/text_backend.h"
#include "player/player.h"
#include "settings/config.h"
#include "sys/input.h"

namespace {

constexpr std::array kGalleryGrid = {
    std::array<uint8_t, 6>{0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
    std::array<uint8_t, 6>{0x10, 0x11, 0x12, 0x13, 0x14, 0x15},
    std::array<uint8_t, 6>{0x20, 0x21, 0x22, 0x23, 0x24, 0x25},
    std::array<uint8_t, 6>{0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF},
    std::array<uint8_t, 6>{0x40, 0x41, 0x42, 0x43, 0xFF, 0xFF},
};
constexpr int kGalleryX = 160;
constexpr int kGalleryY = 50;
constexpr int kGalleryDx = 64;
constexpr int kGalleryDy = 80;

} // namespace

bool BulletGalleryScene::Enter() {
  if (!graphics_.LoadBulletGallery()) {
    return false;
  }
  bullets_.Init();
  bullets_.Clear();
  TextObj.Clear();
  help_text_ = TextObj.Register({.w = 480, .h = 20});
  for (int row = 0; row < 5; row++) {
    for (int column = 0; column < 6; column++) {
      const auto type = kGalleryGrid[row][column];
      if (type != 0xFF) {
        bullets_.PlaceDisplayBullet(
            PixelToWorld(kGalleryX + column * kGalleryDx),
            PixelToWorld(kGalleryY + row * kGalleryDy), type);
      }
    }
  }
  return true;
}

BulletGallerySceneResult BulletGalleryScene::Update(INPUT_BITS input,
                                                    bool should_draw) {
  if ((input & KEY_ESC) != 0U) {
    bullets_.Clear();
    return BulletGallerySceneResult::ExitRequested;
  }
  bullets_.RotateDisplayAngles();
  if (!should_draw) {
    return BulletGallerySceneResult::Running;
  }

  GrpBackend_Clear();
  bullets_.Render();
  if (config_.debug.hitbox_display != 0) {
    bullets_.RenderDebugHitboxes(config_.debug.hitbox_display);
    player_.DrawDebugHitbox();
  }
  for (int row = 0; row < 5; row++) {
    for (int column = 0; column < 6; column++) {
      const auto type = kGalleryGrid[row][column];
      if (type != 0xFF) {
        const auto label = std::format("{:02X}", type);
        GrpPut16(kGalleryX + column * kGalleryDx - 4,
                 kGalleryY + row * kGalleryDy + 16, label.c_str());
      }
    }
  }
  GrpBackend_SetClip(GRP_RES_RECT);
  const auto help =
      localization_.Text(i18n::TextIdFromKey("ui.bullet_gallery.exit_help"));
  TextObj.Render({80, 458}, help_text_, help, [help](TEXTRENDER_SESSION &s) {
    s.SetFont(FONT_ID::NORMAL);
    const auto x = TextLayoutXCenter(s, help);
    s.Put({x + 1, 1}, help, RGB{96, 96, 96});
    s.Put({x, 0}, help, RGB{255, 255, 255});
  });
  GrpBackend_SetClip({playfield::kLeft, playfield::kTop, playfield::kRight + 1,
                      playfield::kBottom + 1});
  Grp_Flip();
  return BulletGallerySceneResult::Running;
}
