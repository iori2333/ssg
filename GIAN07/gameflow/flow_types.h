/// Semantic events exchanged between game-flow states.

#pragma once

#include <string>
#include <variant>

#include "gameplay/game_rules.h"
#include "sys/input.h"

namespace gameflow {

struct NoEvent {};
struct QuitRequested {};
struct ReturnToTitle {
  bool change_music;
};
struct StartWeaponSelect {
  bool extra_stage;
};
struct StartLiveGame {};
struct StartDemo {};
struct StartReplay {
  std::string path;
  StageId stage;
};
struct OpenReplayBrowser {};
struct OpenScoreBrowser {
  GameLevel difficulty;
};
struct OpenMusicRoom {};
struct OpenBulletGallery {};
struct OpenEnding {};
struct FinishRun {
  bool extra_stage;
  bool change_music;
  bool save_replay;
};
struct SaveReplayAndExit {
  bool extra_stage;
};

using FlowEvent =
    std::variant<NoEvent, QuitRequested, ReturnToTitle, StartWeaponSelect,
                 StartLiveGame, StartDemo, StartReplay, OpenReplayBrowser,
                 OpenScoreBrowser, OpenMusicRoom, OpenBulletGallery, OpenEnding,
                 FinishRun, SaveReplayAndExit>;

struct FrameInput {
  INPUT_BITS gameplay;
  INPUT_BITS system;
  bool should_draw;
};

} // namespace gameflow
