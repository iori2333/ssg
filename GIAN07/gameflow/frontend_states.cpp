/// Title-side game-flow states.

#include <algorithm>
#include <array>
#include <format>
#include <string_view>

#include "frontend_states.h"
#include "gameplay_state.h"

#include "app/game_context.h"
#include "audio/bgm.h"
#include "audio/snd.h"
#include "audio/snd_backend.h"
#include "gameplay/playfield.h"
#include "gfx/constants.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "platform/text_backend.h"
#include "sys/input.h"
#include "util/cast.h"
#include "util/ut_math.h"

namespace gameflow {
namespace {

constexpr WINDOW_POINT kMainWindowTopLeft = {400, 250};
constexpr auto kBuildLabel = "BUILD";

constexpr uint8_t PlayerTypeIndex(PlayerType type) {
  return std::to_underlying(type);
}

} // namespace

bool ProjectState::Enter(GameContext &context) {
  GrpBackend_PixelAccessStart();
  if (!context.graphics.LoadProjectScreen()) {
    return false;
  }
  lens_ = GrpCreateLensBall(70, 36);
  timer_ = 0;
  return lens_.has_value();
}

FlowEvent ProjectState::Update(GameContext & /*context*/,
                               const FrameInput &frame) {
  constexpr PIXEL_SIZE logo_size = {.w = 320, .h = 42};
  constexpr WINDOW_LTRB logo = WINDOW_LTWH{
      (320 - (logo_size.w / 2)), (240 + 40), logo_size.w, logo_size.h};

  timer_++;
  if (timer_ >= 256) {
    lens_.reset();
    return ReturnToTitle{.change_music = true};
  }
  if (!frame.should_draw) {
    return NoEvent{};
  }

  GrpBackend_Clear();
  constexpr PIXEL_LTRB rc = {0, 0, logo_size.w, logo_size.h};
  GrpSurface_Blit({logo.left, logo.top}, SURFACE_ID::SPROJECT, rc);

  const auto fade = [logo](uint8_t black_alpha) {
    if (auto *geometry = GrpGeom_Poly()) {
      geometry->Lock();
      geometry->SetAlphaNorm(black_alpha);
      geometry->SetColor({0, 0, 0});
      geometry->DrawBoxA(logo.left, logo.top, logo.right, logo.bottom);
      geometry->Unlock();
    }
  };

  if (timer_ < 64) {
    fade((255 - timer_) * 4);
  } else if (timer_ > 192) {
    fade(timer_ * 4);
  } else if (lens_) {
    const uint8_t d = timer_ - 64;
    lens_->Draw({320 + sinl(d - 64, 240), 295 + sinl(d * 2, 20)});
  }
  Grp_Flip();
  return NoEvent{};
}

bool TitleState::Enter(GameContext &context, INPUT_BITS initial_input,
                       bool change_music) {
  SndBackend_ResumeAll();
  GrpBackend_PixelAccessEnd();
  TextObj.Clear();
  GrpBackend_Clear();
  Grp_Flip();
  if (!context.graphics.LoadTitle()) {
    return false;
  }
  GrpBackend_SetClip(GRP_RES_RECT);
  Snd_SEStop(8);

  context.ui.ForceCloseMessageWindow();
  context.ui.InitMessageWindow(
      {(128 + 8), (400 + 16 + 20), (640 - 128 - 8), 480},
      MsgWindowFlags::CENTER);
  context.ui.OpenMessageWindow();

  demo_timer_ = 0;
  context.session.stage = StageId::Stage1;
  if (change_music) {
    context.music.Play(0);
  }

  context.ui.InitMain(context.config, {.graphics = context.graphics,
                                       .sound_effects = context.sound_effects,
                                       .music = context.music});
  context.ui.Main().Open(kMainWindowTopLeft, 0, initial_input);
  InitVersion();
  return true;
}

void TitleState::InitVersion() {
  const auto build_w = TextObj.TextExtent(FONT_ID::TINY, VERSION_TAG).w;
  version_rect_ = TextObj.Register({.w = 136, .h = 10});
  version_left_ = GRP_RES.w - build_w;
}

void TitleState::DrawVersion(PIXEL_COORD top) const {
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
  const WINDOW_POINT label_position = {version_left_ - label_extent.w,
                                       top + 2 + 7 - label_extent.h};
  GrpPut55(label_position, kBuildLabel);
  TextObj.Render({version_left_, top}, version_rect_, VERSION_TAG,
                 [gradient](auto &session) {
                   std::array<std::string_view, 1> text = {VERSION_TAG};
                   DrawGrdFont(session, {text}, FONT_ID::TINY, false, gradient);
                 });
}

FlowEvent TitleState::Update(GameContext &context, const FrameInput &frame) {
  constexpr PIXEL_LTRB src = {0, 0, 640, 396};
  BGM_UpdateMIDITables();

  if (frame.gameplay == 0) {
    demo_timer_++;
  } else {
    demo_timer_ = 0;
  }
  if (context.ui.Main().Depth() > 1) {
    demo_timer_ = 0;
  }
  if (demo_timer_ == 60 * 10) {
    return StartDemo{};
  }

  auto *active_menu = context.ui.ActiveMenu();
  active_menu->Tick(frame.gameplay);
  context.ui.ShowMenuHelp();
  context.ui.TickMessageWindow();

  if (const auto requested_action = context.ui.TakeMainMenuAction()) {
    const auto action = *requested_action;
    switch (action) {
    case menu::MainMenuAction::StartGame:
      return StartWeaponSelect{.extra_stage = false};
    case menu::MainMenuAction::StartExtra:
      return StartWeaponSelect{.extra_stage = true};
    case menu::MainMenuAction::OpenReplay:
      return OpenReplayBrowser{};
    case menu::MainMenuAction::OpenScore:
      return OpenScoreBrowser{
          .difficulty = context.session.stage == StageId::Extra
                            ? GameLevel::Extra
                            : context.session.level,
      };
    case menu::MainMenuAction::OpenMusicRoom:
      return OpenMusicRoom{};
    case menu::MainMenuAction::OpenBulletGallery:
      return OpenBulletGallery{};
    }
  }

  if (!context.ui.Main().Active()) {
    return QuitRequested{};
  }
  context.ui.Main().AdjustYForTallMenu(kMainWindowTopLeft.y, 9);

  if (frame.should_draw) {
    GrpBackend_Clear();
    GrpSurface_Blit({0, 42}, SURFACE_ID::TITLE, src);
    context.ui.DrawMessageWindow();
    active_menu->Draw();
    DrawVersion(438);
    Grp_Flip();
  }
  return NoEvent{};
}

bool WeaponSelectState::Enter(GameContext &context, bool extra_stage) {
  GrpBackend_Clear();
  Grp_Flip();
  context.session.level =
      extra_stage ? GameLevel::Extra : context.config.game.game_level;
  ResetGameplayRuntime(context);
  context.session.ResetRank();
  context.player.SelectType(PlayerType::Wide);
  context.player.Initialize(context.config.game.player_stock,
                            context.config.game.bomb_stock);
  GrpBackend_SetClip(GRP_RES_RECT);
  key_wait_ = 1;
  count_ = 0;
  angle_ = 0;
  speed_ = 0;
  if (extra_stage) {
    context.session.stage = StageId::Extra;
  }
  return true;
}

FlowEvent WeaponSelectState::Update(GameContext &context,
                                    const FrameInput &frame) {
  constexpr std::array sprites = {
      PIXEL_LTWH{0, 344, 56, 48},
      PIXEL_LTWH{0, 392, 56, 48},
      PIXEL_LTWH{56, 344, 56, 48},
      PIXEL_LTWH{56, 392, 56, 48},
  };

  angle_ += speed_;
  if (angle_ >= 85 || angle_ <= -85) {
    context.player.RotateType(speed_ < 0 ? -1 : 1);
    speed_ = 0;
    angle_ = 0;
    Snd_SEPlay(SfxId::Buzz);
  }

  auto input = frame.gameplay;
  if (key_wait_ != 0U) {
    if (input == 0U) {
      key_wait_ = 0;
    } else {
      input = 0;
    }
  }

  int forced_stage = 0;
  if ((input & KEY_STAGE1) != 0) {
    forced_stage = 1;
  } else if ((input & KEY_STAGE2) != 0) {
    forced_stage = 2;
  } else if ((input & KEY_STAGE3) != 0) {
    forced_stage = 3;
  } else if ((input & KEY_STAGE4) != 0) {
    forced_stage = 4;
  } else if ((input & KEY_STAGE5) != 0) {
    forced_stage = 5;
  } else if ((input & KEY_STAGE6) != 0) {
    forced_stage = 6;
  }
  input &= ~(KEY_STAGE1 | KEY_STAGE2 | KEY_STAGE3 | KEY_STAGE4 | KEY_STAGE5 |
             KEY_STAGE6);
  const auto shift_held = input & KEY_SHIFT;
  input &= ~KEY_SHIFT;

  switch (input) {
  case KEY_RIGHT:
    if (speed_ < 0) {
      context.player.RotateType(-1);
      angle_ += 85;
    }
    speed_ = 3;
    break;
  case KEY_LEFT:
    if (speed_ > 0) {
      context.player.RotateType(1);
      angle_ -= 85;
    }
    speed_ = -3;
    break;
  case KEY_TAMA:
  case KEY_RETURN:
    if (speed_ != 0) {
      break;
    }
    context.player.Initialize(context.config.game.player_stock,
                              context.config.game.bomb_stock);
    count_ = 0;
    Snd_SEPlay(SfxId::Select);
    if (context.session.stage != StageId::Extra) {
      if (forced_stage != 0) {
        context.session.stage = static_cast<StageId>(forced_stage - 1);
        if (context.session.stage == StageId::Stage2) {
          context.player.SetPower(160);
        }
        if (context.session.stage >= StageId::Stage3) {
          context.player.SetPower(255);
        }
      } else {
        context.session.stage = StageId::Stage1;
      }
    } else {
      context.player.SetCredits(0);
      context.player.SetLives(2);
      context.player.SetPower(255);
    }
    context.records.BeginRecording(context.player, context.session,
                                   context.config);
    return StartLiveGame{};
  case KEY_ESC:
  case KEY_BOMB:
    if (speed_ == 0) {
      Snd_SEPlay(SfxId::Cancel);
      return ReturnToTitle{.change_music = false};
    }
    break;
  default:
    break;
  }

  count_ = (count_ + 1) % (256 + 128);
  if (!frame.should_draw) {
    return NoEvent{};
  }

  GrpBackend_Clear();
  GrpSurface_Blit({320 - 112, 20}, SURFACE_ID::SYSTEM,
                  {0, 264 - 8, 224, 296 - 24});
  GrpSurface_Blit({120 - 32, 260 - 12}, SURFACE_ID::SYSTEM,
                  PIXEL_LTWH{0, 272, 64, 24});
  const uint8_t prompt_offset = ((count_ / 8) % 2) << 3;
  GrpSurface_Blit({400 - 28 + 4, 420}, SURFACE_ID::SYSTEM,
                  PIXEL_LTWH{72, 272 + prompt_offset, 56, 8});

  for (int i = 0; i < 3; i++) {
    const int d =
        (-i + PlayerTypeIndex(context.player.Type())) * 85 + angle_ - 64;
    const int x = 120 + cosl(d, 90) - 56 / 2;
    const int y = 260 + sinl(d, 110) - 48 / 2;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, sprites[i]);
  }

  GrpGeom->Lock();
  GrpGeom->SetColor({0, 0, 1});
  GrpGeom->SetAlphaNorm(128);
  for (int i = 0; i < 3; i++) {
    if (context.session.stage != StageId::Extra ||
        ((1 << i) & context.session.extra_stg_flags) != 0) {
      continue;
    }
    const int d =
        (-i + PlayerTypeIndex(context.player.Type())) * 85 + angle_ - 64;
    const int x = 120 + cosl(d, 90) - 56 / 2;
    const int y = 260 + sinl(d, 110) - 48 / 2;
    GrpGeom->DrawBoxA(x, y, x + 56, y + 48);
  }
  GrpGeom->Unlock();

  context.player.SetPower(static_cast<uint8_t>(std::min(count_, 255)));
  if (context.player.Power() < 31) {
    context.player.ClearContinuousAttack();
  }
  context.enemies.ResetHomingTarget();
  context.player.ClearInvincibility();
  context.player.SetPosition(400_px + sinl((count_ / 3) * 6, 60_px),
                             350_px + sinl((count_ / 3) * 4, 30_px));
  static_cast<void>(
      context.player.Update(context.enemies, KEY_TAMA | shift_held));

  GrpBackend_SetClip({400 - 110, 400 - 300 + 2, 400 + 110, 400 + 10});
  for (int x = 400 - 110 - 2; x < 400 + 110; x += 32) {
    for (int y = 400 - 300 + 2 + ((count_ * 2) % 32) - 32; y < 400 + 10;
         y += 32) {
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, PIXEL_LTWH{224, 256, 32, 32});
    }
  }
  context.player.Draw();
  context.player.DrawProjectiles();
  GrpSurface_Blit({468, 400}, SURFACE_ID::SYSTEM, PIXEL_LTWH{72, 288, 56, 8});
  GrpPutScore(
      500, 400,
      std::format("{}", (Cast::up<uint16_t>(context.player.Power()) + 1) >> 5)
          .c_str());

  GrpBackend_SetClip(GRP_RES_RECT);
  GrpGeom->Lock();
  GrpGeom->SetColor({0, 0, 4});
  GrpGeom->DrawLine(290, 100, 510, 100);
  GrpGeom->DrawLine(290, 410, 510, 410);
  GrpGeom->DrawLine(290, 100, 290, 410);
  GrpGeom->DrawLine(510, 100, 510, 410);
  if (std::abs(angle_) <= 25) {
    GrpGeom->SetColor({2, 2, 5});
    GeomCircle({120, 150}, 49 - 2 * std::abs(angle_));
    GrpGeom->SetColor({4, 4, 5});
    GeomCircle({120, 150}, 51 - 2 * std::abs(angle_));
  }
  GrpGeom->Unlock();
  Grp_Flip();
  return NoEvent{};
}

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

bool BulletGalleryState::Enter(GameContext &context) {
  if (!context.graphics.LoadBulletGallery()) {
    return false;
  }
  context.bullets.Init();
  context.bullets.Clear();
  for (int row = 0; row < 5; row++) {
    for (int column = 0; column < 6; column++) {
      const auto type = kGalleryGrid[row][column];
      if (type != 0xFF) {
        context.bullets.PlaceDisplayBullet(
            PixelToWorld(kGalleryX + column * kGalleryDx),
            PixelToWorld(kGalleryY + row * kGalleryDy), type);
      }
    }
  }
  return true;
}

FlowEvent BulletGalleryState::Update(GameContext &context,
                                     const FrameInput &frame) {
  if ((frame.gameplay & KEY_ESC) != 0U) {
    context.bullets.Clear();
    return ReturnToTitle{.change_music = false};
  }
  context.bullets.RotateDisplayAngles();
  if (!frame.should_draw) {
    return NoEvent{};
  }

  GrpBackend_Clear();
  context.bullets.Render();
  if (context.config.debug.hitbox_display != 0) {
    context.bullets.RenderDebugHitboxes(context.config.debug.hitbox_display);
    context.player.DrawDebugHitbox();
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
  GrpPut16(140, 460, "Bullet Gallery  |  ESC to exit");
  GrpBackend_SetClip({playfield::kLeft, playfield::kTop, playfield::kRight + 1,
                      playfield::kBottom + 1});
  Grp_Flip();
  return NoEvent{};
}

} // namespace gameflow
