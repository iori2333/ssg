/// Player weapon selection scene.

#pragma once

#include <cstdint>

#include "sys/input.h"

class EnemyManager;
class GameSession;
class Player;
struct ConfigData;

enum class WeaponSelectSceneResult : uint8_t {
  Running,
  StartGame,
  Cancelled,
};

class WeaponSelectScene {
public:
  WeaponSelectScene(const ConfigData &config, EnemyManager &enemies,
                    GameSession &session, Player &player)
      : config_(config), enemies_(enemies), session_(session), player_(player) {
  }

  void Enter();
  [[nodiscard]] WeaponSelectSceneResult Update(INPUT_BITS input,
                                               bool should_draw);
  void DrawPreview(INPUT_BITS preview_input = KEY_TAMA);
  void PrepareGameStart();

private:
  const ConfigData &config_;
  EnemyManager &enemies_;
  GameSession &session_;
  Player &player_;
  int count_ = 0;
  int angle_ = 0;
  int speed_ = 0;
  uint8_t key_wait_ = 0;
};
