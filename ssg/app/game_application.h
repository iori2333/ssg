/// Top-level application lifetime and composition root.

#pragma once

#include <memory>

#include "game_context.h"

#include "sys/input.h"

namespace gameflow {
class GameFlow;
}

class GameApplication {
public:
  GameApplication();
  ~GameApplication();
  GameApplication(const GameApplication &) = delete;
  GameApplication(GameApplication &&) = delete;
  GameApplication &operator=(const GameApplication &) = delete;
  GameApplication &operator=(GameApplication &&) = delete;

  [[nodiscard]] bool Initialize();
  [[nodiscard]] int Run();

private:
  [[nodiscard]] bool Tick();
  void Shutdown();
  void PersistConfig();

  ConfigData config_;
  InputSystem input_;
  InputSnapshot input_snapshot_;
  GameContext context_{config_, [this] { PersistConfig(); }};
  std::unique_ptr<gameflow::GameFlow> flow_;
  bool config_loaded_ = false;
  bool display_initialized_ = false;
  bool running_ = false;
};
