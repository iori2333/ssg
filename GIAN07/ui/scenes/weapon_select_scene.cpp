/// Player weapon selection scene.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <utility>

#include "weapon_select_scene.h"

#include "audio/constants.h"
#include "audio/snd.h"
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
#include "util/cast.h"
#include "util/ut_math.h"

namespace {

constexpr uint8_t PlayerTypeIndex(PlayerType type) {
  return std::to_underlying(type);
}

} // namespace

void WeaponSelectScene::Enter() {
  GrpBackend_SetClip(GRP_RES_RECT);
  key_wait_ = 1;
  count_ = 0;
  angle_ = 0;
  speed_ = 0;
}

WeaponSelectSceneResult WeaponSelectScene::Update(INPUT_BITS input,
                                                  bool should_draw) {
  constexpr std::array sprites = {
      PIXEL_LTWH{0, 344, 56, 48},
      PIXEL_LTWH{0, 392, 56, 48},
      PIXEL_LTWH{56, 344, 56, 48},
      PIXEL_LTWH{56, 392, 56, 48},
  };

  angle_ += speed_;
  if (angle_ >= 85 || angle_ <= -85) {
    player_.RotateType(speed_ < 0 ? -1 : 1);
    speed_ = 0;
    angle_ = 0;
    Snd_SEPlay(SfxId::Buzz);
  }

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
      player_.RotateType(-1);
      angle_ += 85;
    }
    speed_ = 3;
    break;
  case KEY_LEFT:
    if (speed_ > 0) {
      player_.RotateType(1);
      angle_ -= 85;
    }
    speed_ = -3;
    break;
  case KEY_TAMA:
  case KEY_RETURN:
    if (speed_ != 0) {
      break;
    }
    if (session_.stage == StageId::Extra &&
        ((1U << PlayerTypeIndex(player_.Type())) &
         config_.progress.extra_stg_flags) == 0) {
      Snd_SEPlay(SfxId::Buzz);
      break;
    }
    player_.Initialize(config_.game.player_stock, config_.game.bomb_stock);
    count_ = 0;
    Snd_SEPlay(SfxId::Select);
    if (session_.stage != StageId::Extra) {
      if (forced_stage != 0) {
        session_.stage = static_cast<StageId>(forced_stage - 1);
        if (session_.stage == StageId::Stage2) {
          player_.SetPower(160);
        }
        if (session_.stage >= StageId::Stage3) {
          player_.SetPower(255);
        }
      } else {
        session_.stage = StageId::Stage1;
      }
    } else {
      player_.SetCredits(0);
      player_.SetLives(2);
      player_.SetPower(255);
    }
    return WeaponSelectSceneResult::StartGame;
  case KEY_ESC:
  case KEY_BOMB:
    if (speed_ == 0) {
      Snd_SEPlay(SfxId::Cancel);
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

  GrpBackend_Clear();
  GrpSurface_Blit({320 - 112, 20}, SURFACE_ID::SYSTEM,
                  {0, 264 - 8, 224, 296 - 24});
  GrpSurface_Blit({120 - 32, 260 - 12}, SURFACE_ID::SYSTEM,
                  PIXEL_LTWH{0, 272, 64, 24});
  const uint8_t prompt_offset = ((count_ / 8) % 2) << 3;
  GrpSurface_Blit({400 - 28 + 4, 420}, SURFACE_ID::SYSTEM,
                  PIXEL_LTWH{72, 272 + prompt_offset, 56, 8});

  for (int i = 0; i < 3; i++) {
    const int direction =
        (-i + PlayerTypeIndex(player_.Type())) * 85 + angle_ - 64;
    const int x = 120 + cosl(direction, 90) - 56 / 2;
    const int y = 260 + sinl(direction, 110) - 48 / 2;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, sprites[i]);
  }

  GrpGeom->Lock();
  GrpGeom->SetColor({0, 0, 1});
  GrpGeom->SetAlphaNorm(128);
  for (int i = 0; i < 3; i++) {
    if (session_.stage != StageId::Extra ||
        ((1U << i) & config_.progress.extra_stg_flags) != 0) {
      continue;
    }
    const int direction =
        (-i + PlayerTypeIndex(player_.Type())) * 85 + angle_ - 64;
    const int x = 120 + cosl(direction, 90) - 56 / 2;
    const int y = 260 + sinl(direction, 110) - 48 / 2;
    GrpGeom->DrawBoxA(x, y, x + 56, y + 48);
  }
  GrpGeom->Unlock();

  player_.SetPower(static_cast<uint8_t>(std::min(count_, 255)));
  if (player_.Power() < 31) {
    player_.ClearContinuousAttack();
  }
  enemies_.ResetHomingTarget();
  player_.ClearInvincibility();
  player_.SetPosition(400_px + sinl((count_ / 3) * 6, 60_px),
                      350_px + sinl((count_ / 3) * 4, 30_px));
  static_cast<void>(player_.Update(enemies_, KEY_TAMA | shift_held));

  GrpBackend_SetClip({400 - 110, 400 - 300 + 2, 400 + 110, 400 + 10});
  for (int x = 400 - 110 - 2; x < 400 + 110; x += 32) {
    for (int y = 400 - 300 + 2 + ((count_ * 2) % 32) - 32; y < 400 + 10;
         y += 32) {
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, PIXEL_LTWH{224, 256, 32, 32});
    }
  }
  player_.Draw();
  player_.DrawProjectiles();
  GrpSurface_Blit({468, 400}, SURFACE_ID::SYSTEM, PIXEL_LTWH{72, 288, 56, 8});
  GrpPutScore(500, 400,
              std::format("{}", (Cast::up<uint16_t>(player_.Power()) + 1) >> 5)
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
  return WeaponSelectSceneResult::Running;
}
