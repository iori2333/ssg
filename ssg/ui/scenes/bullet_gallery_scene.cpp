/// Debug bullet gallery scene.

#include <array>
#include <cstdint>
#include <format>
#include <string>

#include "bullet_gallery_scene.h"

#include "bullet/bullet_manager.h"
#include "data/graphics_loader.h"
#include "gameplay/playfield.h"
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/graphics.h"
#include "gfx/text/text.h"
#include "gfx/text/text_renderer.h"
#include "i18n/localization.h"
#include "player/player.h"
#include "settings/config.h"
#include "sys/input.h"
#include "ui/bitmap_font.h"

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
  TextRenderer().Clear();
  help_text_ = TextRenderer().Register({.x = 480, .y = 20});
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

BulletGallerySceneResult BulletGalleryScene::Update(InputBits input,
                                                    bool should_draw) {
  if ((input & KeyEscape) != 0U) {
    bullets_.Clear();
    return BulletGallerySceneResult::ExitRequested;
  }
  bullets_.RotateDisplayAngles();
  if (!should_draw) {
    return BulletGallerySceneResult::Running;
  }

  GraphicsClear();
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
        ui::Draw16({kGalleryX + column * kGalleryDx - 4,
                    kGalleryY + row * kGalleryDy + 16},
                   label);
      }
    }
  }
  GraphicsSetClip(kGameResolutionRect);
  const auto help =
      localization_.Text(i18n::TextIdFromKey("ui.bullet_gallery.exit_help"));
  TextRenderer().Render(
      {80, 458}, help_text_, help, [help](TextRenderSession &s) {
        s.SetFont(FontId::Normal);
        const auto x = TextLayoutXCenter(s, help);
        s.Put({.x = x + 1, .y = 1}, help, Rgb{.r = 96, .g = 96, .b = 96});
        s.Put({.x = x, .y = 0}, help, Rgb{.r = 255, .g = 255, .b = 255});
      });
  GraphicsSetClip({playfield::kLeft, playfield::kTop, playfield::kRight + 1,
                   playfield::kBottom + 1});
  GraphicsFlip();
  return BulletGallerySceneResult::Running;
}
