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
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/graphics.h"
#include "gfx/text/text_renderer.h"
#include "music/music_player.h"
#include "sys/input.h"
#include "ui/bitmap_font.h"
#include "ui/menu/menu_builder.h"
#include "ui/msg_window/msg_window.h"
#include "ui/ui_manager.h"

namespace {

constexpr PixelPoint kMainWindowTopLeft = {400, 250};
constexpr auto kBuildLabel = "BUILD";
constexpr int kBuildTagGap = 4;

} // namespace

bool TitleScene::Enter(InputBits initial_input, bool change_music) {
  audio_.ResumeAll();
  GraphicsPixelAccessEnd();
  TextRenderer().Clear();
  GraphicsClear();
  GraphicsFlip();
  if (!graphics_.LoadTitle()) {
    return false;
  }
  GraphicsSetClip(kGameResolutionRect);
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
  const auto build_width = TextRender::TextExtent(FontId::Tiny, kVersionTag).x;
  version_rect_ = TextRenderer().Register({.x = 136, .y = 10});
  version_left_ = kGameResolution.x - build_width;
}

void TitleScene::DrawVersion(int top) const {
  const auto gradient = [](int y) -> uint8_t {
    if (y <= 3) {
      return 254;
    }
    if (y <= 6) {
      return 220;
    }
    return 180;
  };
  constexpr auto label_extent = ui::TinyExtent(kBuildLabel);
  const PixelPoint label_position = {version_left_ - label_extent.x -
                                         kBuildTagGap,
                                     top + 2 + 7 - label_extent.y};
  ui::DrawTinyUpper(label_position, kBuildLabel);
  TextRenderer().Render({version_left_, top}, version_rect_, kVersionTag,
                        [gradient](auto &session) {
                          std::array<std::string_view, 1> text = {kVersionTag};
                          ui::DrawGradient(session, {text}, FontId::Tiny, false,
                                           gradient);
                        });
}

TitleSceneResult TitleScene::Update(InputBits input, bool should_draw) {
  constexpr Rect source = {0, 0, 640, 396};

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
    GraphicsClear();
    GraphicsSurfaceBlit({0, 42}, SurfaceId::Title, source);
    ui_.DrawMessageWindow();
    main_menu.Draw();
    DrawVersion(438);
    GraphicsFlip();
  }
  return TitleSceneResult::Running;
}
