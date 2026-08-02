/// Player weapon selection scene.

#pragma once

#include <cstdint>

#include "sys/input.h"

class EnemyManager;
class GameSession;
class Player;
struct ConfigData;

namespace audio {
class AudioSystem;
}

enum class WeaponSelectSceneResult : uint8_t {
  Running,
  StartGame,
  Cancelled,
};

class WeaponSelectScene {
public:
  WeaponSelectScene(const ConfigData &config, EnemyManager &enemies,
                    GameSession &session, Player &player,
                    audio::AudioSystem &audio)
      : config_(config), enemies_(enemies), session_(session), player_(player),
        audio_(audio) {}

  void Enter();
  [[nodiscard]] WeaponSelectSceneResult Update(InputBits input,
                                               bool should_draw);
  void DrawPreview(InputBits preview_input = KeyTama);
  void PrepareGameStart();

private:
  const ConfigData &config_;
  EnemyManager &enemies_;
  GameSession &session_;
  Player &player_;
  audio::AudioSystem &audio_;
  int count_ = 0;
  int angle_ = 0;
  int speed_ = 0;
  uint8_t key_wait_ = 0;
};
