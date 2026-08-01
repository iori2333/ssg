/// Player weapon selection scene.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <utility>

#include "weapon_select_scene.h"

#include "audio/sfx.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "player/loadout/player_loadout.h"
#include "player/player.h"
#include "settings/config.h"
#include "sys/input.h"
#include "util/math_utils.h"

namespace {

constexpr uint8_t PlayerTypeIndex(PlayerType type) {
  return std::to_underlying(type);
}

} // namespace

void WeaponSelectScene::Enter() {
  GraphicsBackendSetClip(kGameResolutionRect);
  key_wait_ = 1;
  count_ = 0;
  angle_ = 0;
  speed_ = 0;
}

WeaponSelectSceneResult WeaponSelectScene::Update(InputBits input,
                                                  bool should_draw) {
  angle_ += speed_;
  if (angle_ >= 85 || angle_ <= -85) {
    player_.RotateType(speed_ < 0 ? -1 : 1);
    speed_ = 0;
    angle_ = 0;
    PlaySfx(SfxId::Buzz);
  }

  if (key_wait_ != 0U) {
    if (input == 0U) {
      key_wait_ = 0;
    } else {
      input = 0;
    }
  }

  int forced_stage = 0;
  if ((input & KeyStage1) != 0) {
    forced_stage = 1;
  } else if ((input & KeyStage2) != 0) {
    forced_stage = 2;
  } else if ((input & KeyStage3) != 0) {
    forced_stage = 3;
  } else if ((input & KeyStage4) != 0) {
    forced_stage = 4;
  } else if ((input & KeyStage5) != 0) {
    forced_stage = 5;
  } else if ((input & KeyStage6) != 0) {
    forced_stage = 6;
  }
  input &=
      ~(KeyStage1 | KeyStage2 | KeyStage3 | KeyStage4 | KeyStage5 | KeyStage6);
  const auto shift_held = input & KeyShift;
  input &= ~KeyShift;

  switch (input) {
  case KeyRight:
    if (speed_ < 0) {
      player_.RotateType(-1);
      angle_ += 85;
    }
    speed_ = 3;
    break;
  case KeyLeft:
    if (speed_ > 0) {
      player_.RotateType(1);
      angle_ -= 85;
    }
    speed_ = -3;
    break;
  case KeyTama:
  case KeyReturn:
    if (speed_ != 0) {
      break;
    }
    if (session_.stage == StageId::Extra &&
        ((1U << PlayerTypeIndex(player_.Type())) &
         config_.progress.extra_stg_flags) == 0) {
      PlaySfx(SfxId::Buzz);
      break;
    }
    PlaySfx(SfxId::Select);
    if (session_.stage != StageId::Extra) {
      if (forced_stage != 0) {
        session_.stage = static_cast<StageId>(forced_stage - 1);
      } else {
        session_.stage = StageId::Stage1;
      }
    }
    return WeaponSelectSceneResult::StartGame;
  case KeyEscape:
  case KeyBomb:
    if (speed_ == 0) {
      PlaySfx(SfxId::Cancel);
      return WeaponSelectSceneResult::Cancelled;
    }
    break;
  default:
    break;
  }

  count_ = (count_ + 1) % (256 + 128);
  if (!should_draw) {
    return WeaponSelectSceneResult::Running;
  }

  DrawPreview(KeyTama | shift_held);
  GraphicsFlip();
  return WeaponSelectSceneResult::Running;
}

void WeaponSelectScene::DrawPreview(InputBits preview_input) {
  constexpr std::array sprites = {
      PixelLtwh{0, 344, 56, 48},
      PixelLtwh{0, 392, 56, 48},
      PixelLtwh{56, 344, 56, 48},
      PixelLtwh{56, 392, 56, 48},
  };

  GraphicsBackendClear();
  GraphicsSurfaceBlit({320 - 112, 20}, SurfaceId::System,
                      {0, 264 - 8, 224, 296 - 24});
  GraphicsSurfaceBlit({120 - 32, 260 - 12}, SurfaceId::System,
                      PixelLtwh{0, 272, 64, 24});
  const uint8_t prompt_offset = ((count_ / 8) % 2) << 3;
  GraphicsSurfaceBlit({400 - 28 + 4, 420}, SurfaceId::System,
                      PixelLtwh{72, 272 + prompt_offset, 56, 8});

  for (int i = 0; i < 3; i++) {
    const int direction =
        (-i + PlayerTypeIndex(player_.Type())) * 85 + angle_ - 64;
    const auto angle = math::AngleFromLegacy(direction);
    const int x = 120 + math::RoundedPolarVector(angle, 90.0f).x - 56 / 2;
    const int y = 260 + math::RoundedPolarVector(angle, 110.0f).y - 48 / 2;
    GraphicsSurfaceBlit({x, y}, SurfaceId::System, sprites[i]);
  }

  Geometry().SetColor({0, 0, 1});
  Geometry().SetAlphaNorm(128);
  for (int i = 0; i < 3; i++) {
    if (session_.stage != StageId::Extra ||
        ((1U << i) & config_.progress.extra_stg_flags) != 0) {
      continue;
    }
    const int direction =
        (-i + PlayerTypeIndex(player_.Type())) * 85 + angle_ - 64;
    const auto angle = math::AngleFromLegacy(direction);
    const int x = 120 + math::RoundedPolarVector(angle, 90.0f).x - 56 / 2;
    const int y = 260 + math::RoundedPolarVector(angle, 110.0f).y - 48 / 2;
    Geometry().DrawBoxA(x, y, x + 56, y + 48);
  }

  player_.SetPower(static_cast<uint8_t>(std::min(count_, 255)));
  if (player_.Power() < 31) {
    player_.ClearContinuousAttack();
  }
  enemies_.ResetHomingTarget();
  player_.ClearInvincibility();
  const int player_x =
      math::RoundedPolarVector(
          static_cast<float>((count_ / 3) * 6) * math::kLegacyAngleStep, 60_px)
          .y;
  const int player_y =
      math::RoundedPolarVector(
          static_cast<float>((count_ / 3) * 4) * math::kLegacyAngleStep, 30_px)
          .y;
  player_.SetPosition(400_px + player_x, 350_px + player_y);
  // The preview only needs the update's visual side effects.
  (void)player_.Update(enemies_, preview_input);

  GraphicsBackendSetClip({400 - 110, 400 - 300 + 2, 400 + 110, 400 + 10});
  for (int x = 400 - 110 - 2; x < 400 + 110; x += 32) {
    for (int y = 400 - 300 + 2 + ((count_ * 2) % 32) - 32; y < 400 + 10;
         y += 32) {
      GraphicsSurfaceBlit({x, y}, SurfaceId::System,
                          PixelLtwh{224, 256, 32, 32});
    }
  }
  player_.Draw();
  player_.DrawProjectiles();
  GraphicsSurfaceBlit({468, 400}, SurfaceId::System, PixelLtwh{72, 288, 56, 8});
  DrawScore(500, 400,
            std::format("{}", (static_cast<uint16_t>(player_.Power()) + 1) >> 5)
                .c_str());

  GraphicsBackendSetClip(kGameResolutionRect);
  Geometry().SetColor({0, 0, 4});
  Geometry().DrawLine(290, 100, 510, 100);
  Geometry().DrawLine(290, 410, 510, 410);
  Geometry().DrawLine(290, 100, 290, 410);
  Geometry().DrawLine(510, 100, 510, 410);
  if (std::abs(angle_) <= 25) {
    Geometry().SetColor({2, 2, 5});
    GeomCircle({120, 150}, 49 - 2 * std::abs(angle_));
    Geometry().SetColor({4, 4, 5});
    GeomCircle({120, 150}, 51 - 2 * std::abs(angle_));
  }
}

void WeaponSelectScene::PrepareGameStart() {
  player_.Initialize(config_.game.player_stock, config_.game.bomb_stock);
  if (session_.stage == StageId::Extra) {
    player_.SetCredits(0);
    player_.SetLives(2);
    player_.SetPower(255);
  } else if (session_.stage == StageId::Stage2) {
    player_.SetPower(160);
  } else if (session_.stage >= StageId::Stage3) {
    player_.SetPower(255);
  }
}
