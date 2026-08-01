///
/// StageSession - owns the active stage timeline and background
///
#include <cstdint>
#include <utility>

#include "stage_session.h"

#include "audio/bgm.h"
#include "audio/sfx.h"
#include "data/graphics_loader.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "i18n/localization.h"
#include "music/music_player.h"
#include "ui/ui_manager.h"

namespace stage {

bool StageSession::Load(std::span<const uint8_t> map,
                        std::span<const uint8_t> scene) {
  SceneRunner next_scene;
  StageBackground next_background;
  if (!next_scene.Load(scene) || !next_background.LoadMap(map)) {
    return false;
  }
  scene_ = std::move(next_scene);
  background_ = std::move(next_background);
  return true;
}

StageTransition StageSession::Update(StageUpdateContext context,
                                     InputBits input) {
  const auto result = RunScene(context, input);
  if (result.advance_frame) {
    scene_.AdvanceFrame();
  }
  background_.Update(context.effects);
  return result.transition;
}

StageSession::SceneStepResult
StageSession::RunScene(StageUpdateContext &context, InputBits input) {
  while (const auto *instruction = scene_.Current()) {
    switch (instruction->opcode) {
    case SceneOpcode::KeyWait:
      if (scene_.KeyReady((input & (KeyTama | KeyReturn | KeyBomb)) != 0)) {
        scene_.Advance();
        break;
      }
      return {.advance_frame = true};

    case SceneOpcode::Time: {
      const auto target = static_cast<uint32_t>(instruction->value);
      if (!scene_.TimeReady(target,
                            (input & (KeyTama | KeyReturn | KeyBomb)) != 0)) {
        return {.advance_frame = true};
      }
      scene_.Advance();
      break;
    }

    case SceneOpcode::Enemy:
      if (context.enemies.BossCount() == 0) {
        context.enemies.SpawnFromScene(instruction->x, instruction->y,
                                       instruction->script_id);
      }
      scene_.Advance();
      break;

    case SceneOpcode::Boss:
      context.enemies.SpawnBoss({instruction->x, instruction->y},
                                instruction->script_id);
      scene_.Advance();
      if (const auto timeout = FindBossTimeout(); timeout > 0) {
        context.enemies.SetBossTimeout(timeout);
      }
      break;

    case SceneOpcode::BossDead:
      context.enemies.KillBosses();
      scene_.Advance();
      break;

    case SceneOpcode::MessageOpen:
      if (!context.messages_disabled) {
        context.ui.OpenMessageWindow();
      }
      scene_.SetMessageActive(true);
      scene_.Advance();
      break;

    case SceneOpcode::MessageClose:
      if (!context.messages_disabled) {
        context.ui.CloseMessageWindow();
      }
      scene_.SetMessageActive(false);
      scene_.Advance();
      break;

    case SceneOpcode::Message:
      context.ui.ShowMessage(instruction->text);
      scene_.Advance();
      break;

    case SceneOpcode::MessageReference:
      for (const auto line : context.localization.Lines(instruction->text_id)) {
        context.ui.ShowMessage(line);
      }
      scene_.Advance();
      break;

    case SceneOpcode::Face:
      context.ui.SetMessageFace(instruction->face_id);
      scene_.Advance();
      break;

    case SceneOpcode::LoadFace:
      (void)context.graphics.LoadFace(instruction->surface_id,
                                      instruction->file_id);
      scene_.Advance();
      break;

    case SceneOpcode::NewPage:
      context.ui.NewMessagePage();
      scene_.Advance();
      break;

    case SceneOpcode::End:
      return {};

    case SceneOpcode::ScrollSpeed:
      background_.SetSpeed(instruction->value);
      scene_.Advance();
      break;

    case SceneOpcode::Music:
      if (!context.session.is_demoplay) {
        BgmStop();
        if (context.music.Play(instruction->track_id)) {
          BgmPlay();
          const auto title =
              context.localization.MusicTitle(instruction->track_id);
          if (!title.empty()) {
            context.effects.SetMusicTitle(460, title);
          }
        }
      }
      scene_.Advance();
      break;

    case SceneOpcode::DeleteEnemies:
      context.enemies.ResetRegular();
      scene_.Advance();
      break;

    case SceneOpcode::Effect:
      ExecuteEffect(instruction->effect, context);
      scene_.Advance();
      break;

    case SceneOpcode::Wait:
      if (instruction->wait_condition == SceneWaitCondition::BossHp) {
        if (context.enemies.BossHpSum() >
            static_cast<uint32_t>(instruction->value)) {
          return {};
        }
      } else if (instruction->wait_condition == SceneWaitCondition::BossCount) {
        if (context.enemies.BossCount() >
            static_cast<uint32_t>(instruction->value)) {
          return {};
        }
      }
      scene_.Advance();
      break;

    case SceneOpcode::StageClear:
      return {.transition = StageTransition::NextStage};

    case SceneOpcode::GameClear:
      return {.transition = StageTransition::GameClear};

    case SceneOpcode::ExtraClear:
      return {.transition = StageTransition::ExtraClear};

    case SceneOpcode::MapPalette:
    case SceneOpcode::EnemyPalette:
      scene_.Advance();
      break;

    case SceneOpcode::Staff:
      return {};
    }
  }
  return {};
}

int32_t StageSession::FindBossTimeout() const {
  int32_t timeout = -1;
  const auto &instructions = scene_.Instructions();
  for (size_t i = scene_.Position(); i < instructions.size(); ++i) {
    const auto &instruction = instructions[i];
    switch (instruction.opcode) {
    case SceneOpcode::BossDead:
    case SceneOpcode::Wait:
    case SceneOpcode::StageClear:
    case SceneOpcode::GameClear:
    case SceneOpcode::End:
      return timeout;
    case SceneOpcode::Time:
      timeout = instruction.value;
      break;
    default:
      break;
    }
  }
  return timeout;
}

void StageSession::ExecuteEffect(SceneEffect effect,
                                 StageUpdateContext &context) {
  switch (effect) {
  case SceneEffect::Warning:
    PlaySfx(SfxId::Warning, playfield::kWorldCenterX, true);
    context.effects.StartBossWarning();
    break;
  case SceneEffect::StopWarning:
    StopSfx(SfxId::Warning);
    break;
  case SceneEffect::FadeMusic:
    BgmFadeOut(120);
    break;
  case SceneEffect::Stage2Boss:
    background_.Command(BackgroundCommand::Stage2Boss, context.effects);
    break;
  case SceneEffect::RasterOn:
    background_.Command(BackgroundCommand::RasterOn, context.effects);
    break;
  case SceneEffect::RasterOff:
    background_.Command(BackgroundCommand::RasterOff, context.effects);
    break;
  case SceneEffect::CircleFadeIn:
    context.effects.StartScreenTransition(ScreenTransition::CircleFadeIn);
    break;
  case SceneEffect::CircleFadeOut:
    context.effects.StartScreenTransition(ScreenTransition::CircleFadeOut);
    break;
  case SceneEffect::Stage3Boss:
    background_.Command(BackgroundCommand::Stage3Boss, context.effects);
    break;
  case SceneEffect::Stage3Reset:
    background_.Command(BackgroundCommand::Stage3Reset, context.effects);
    break;
  case SceneEffect::Stage6Cube:
    background_.Command(BackgroundCommand::Stage6Cube, context.effects);
    break;
  case SceneEffect::Stage6RandomEcl:
    background_.Command(BackgroundCommand::Stage6RandomEcl, context.effects);
    break;
  case SceneEffect::Stage4Rock:
    background_.Command(BackgroundCommand::Stage4Rock, context.effects);
    break;
  case SceneEffect::Stage4Leave:
    background_.Command(BackgroundCommand::Stage4Leave, context.effects);
    break;
  case SceneEffect::WhiteIn:
    context.effects.StartScreenTransition(ScreenTransition::WhiteIn);
    break;
  case SceneEffect::WhiteOut:
    context.effects.StartScreenTransition(ScreenTransition::WhiteOut);
    break;
  case SceneEffect::LoadExtraBoss1:
    (void)context.graphics.SwapEnemySurface(29);
    break;
  case SceneEffect::LoadExtraBoss2:
    (void)context.graphics.SwapEnemySurface(30);
    break;
  case SceneEffect::Stage6Raster:
    background_.Command(BackgroundCommand::Stage6Raster, context.effects);
    break;
  }
}

} // namespace stage
