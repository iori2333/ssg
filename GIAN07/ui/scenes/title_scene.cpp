/// Title screen scene.

#include <array>
#include <cstdint>
#include <string_view>

#include "title_scene.h"

#include "audio/bgm.h"
#include "audio/snd.h"
#include "audio/snd_backend.h"
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

constexpr WINDOW_POINT kMainWindowTopLeft = {400, 250};
constexpr auto kBuildLabel = "BUILD";
constexpr PIXEL_COORD kBuildTagGap = 4;

} // namespace

bool TitleScene::Enter(INPUT_BITS initial_input, bool change_music) {
  SndBackend_ResumeAll();
  GrpBackend_PixelAccessEnd();
  TextObj.Clear();
  GrpBackend_Clear();
  Grp_Flip();
  if (!graphics_.LoadTitle()) {
    return false;
  }
  GrpBackend_SetClip(GRP_RES_RECT);
  Snd_SEStop(SfxId::Warning);

  ui_.ForceCloseMessageWindow();
  ui_.InitMessageWindow({(128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480},
                        MsgWindowFlags::CENTER);
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
  const auto build_width = TextObj.TextExtent(FONT_ID::TINY, VERSION_TAG).w;
  version_rect_ = TextObj.Register({.w = 136, .h = 10});
  version_left_ = GRP_RES.w - build_width;
}

void TitleScene::DrawVersion(PIXEL_COORD top) const {
  const auto gradient = [](PIXEL_COORD y) -> uint8_t {
    if (y <= 3) {
      return 254;
    }
    if (y <= 6) {
      return 220;
    }
    return 180;
  };
  constexpr auto label_extent = GrpExtent5(kBuildLabel);
  const WINDOW_POINT label_position = {version_left_ - label_extent.w -
                                           kBuildTagGap,
                                       top + 2 + 7 - label_extent.h};
  GrpPut55(label_position, kBuildLabel);
  TextObj.Render({version_left_, top}, version_rect_, VERSION_TAG,
                 [gradient](auto &session) {
                   std::array<std::string_view, 1> text = {VERSION_TAG};
                   DrawGrdFont(session, {text}, FONT_ID::TINY, false, gradient);
                 });
}

TitleSceneResult TitleScene::Update(INPUT_BITS input, bool should_draw) {
  constexpr PIXEL_LTRB source = {0, 0, 640, 396};
  BGM_UpdateMIDITables();

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
    GrpBackend_Clear();
    GrpSurface_Blit({0, 42}, SURFACE_ID::TITLE, source);
    ui_.DrawMessageWindow();
    main_menu.Draw();
    DrawVersion(438);
    Grp_Flip();
  }
  return TitleSceneResult::Running;
}
