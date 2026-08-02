/// Title screen scene.

#include <array>
#include <cstdint>
#include <string_view>

#include "title_scene.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "data/graphics_loader.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "music/music_player.h"
#include "platform/text_backend.h"
#include "sys/input.h"
#include "ui/menu/menu_builder.h"
#include "ui/msg_window/msg_window.h"
#include "ui/ui_manager.h"

namespace {

constexpr WindowPoint kMainWindowTopLeft = {400, 250};
constexpr auto kBuildLabel = "BUILD";
constexpr PixelCoord kBuildTagGap = 4;

} // namespace

bool TitleScene::Enter(InputBits initial_input, bool change_music) {
  audio_.ResumeAll();
  GraphicsBackendPixelAccessEnd();
  TextRenderer().Clear();
  GraphicsBackendClear();
  GraphicsFlip();
  if (!graphics_.LoadTitle()) {
    return false;
  }
  GraphicsBackendSetClip(kGameResolutionRect);
  audio_.StopSfx(SfxId::Warning);

  ui_.ForceCloseMessageWindow();
  ui_.InitMessageWindow({(128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480},
                        MsgWindowFlags::Center);
  ui_.OpenMessageWindow();

  demo_timer_ = 0;
  session_.stage = StageId::Stage1;
  if (change_music) {
    music_.Play(0);
  }

  ui_.InitMain();
  ui_.Main().Open(kMainWindowTopLeft, 0, initial_input);
  InitVersion();
  return true;
}

void TitleScene::InitVersion() {
  const auto build_width = TextRender::TextExtent(FontId::Tiny, kVersionTag).w;
  version_rect_ = TextRenderer().Register({.w = 136, .h = 10});
  version_left_ = kGameResolution.w - build_width;
}

void TitleScene::DrawVersion(PixelCoord top) const {
  const auto gradient = [](PixelCoord y) -> uint8_t {
    if (y <= 3) {
      return 254;
    }
    if (y <= 6) {
      return 220;
    }
    return 180;
  };
  constexpr auto label_extent = FontExtent5(kBuildLabel);
  const WindowPoint label_position = {version_left_ - label_extent.w -
                                          kBuildTagGap,
                                      top + 2 + 7 - label_extent.h};
  DrawFont55(label_position, kBuildLabel);
  TextRenderer().Render({version_left_, top}, version_rect_, kVersionTag,
                        [gradient](auto &session) {
                          std::array<std::string_view, 1> text = {kVersionTag};
                          DrawGrdFont(session, {text}, FontId::Tiny, false,
                                      gradient);
                        });
}

TitleSceneResult TitleScene::Update(InputBits input, bool should_draw) {
  constexpr PixelLtrb source = {0, 0, 640, 396};

  if (input == 0) {
    demo_timer_++;
  } else {
    demo_timer_ = 0;
  }
  if (ui_.Main().Depth() > 1 || ui_.Main().InListView()) {
    demo_timer_ = 0;
  }
  if (demo_timer_ == 60 * 10) {
    return TitleSceneResult::StartDemo;
  }

  auto &main_menu = ui_.Main();
  main_menu.Tick(input);
  ui_.ShowMenuHelp();
  ui_.TickMessageWindow();

  if (const auto requested_action = ui_.TakeMainMenuAction()) {
    switch (*requested_action) {
    case menu::MainMenuAction::StartGame:
      return TitleSceneResult::StartGame;
    case menu::MainMenuAction::StartExtra:
      return TitleSceneResult::StartExtra;
    case menu::MainMenuAction::OpenReplay:
      return TitleSceneResult::OpenReplay;
    case menu::MainMenuAction::OpenScore:
      return TitleSceneResult::OpenScore;
    case menu::MainMenuAction::OpenMusicRoom:
      return TitleSceneResult::OpenMusicRoom;
    case menu::MainMenuAction::OpenBulletGallery:
      return TitleSceneResult::OpenBulletGallery;
    }
  }

  if (!ui_.Main().Active()) {
    return TitleSceneResult::QuitRequested;
  }
  ui_.Main().AdjustYForTallMenu(kMainWindowTopLeft.y, 9);

  if (should_draw) {
    GraphicsBackendClear();
    GraphicsSurfaceBlit({0, 42}, SurfaceId::Title, source);
    ui_.DrawMessageWindow();
    main_menu.Draw();
    DrawVersion(438);
    GraphicsFlip();
  }
  return TitleSceneResult::Running;
}
