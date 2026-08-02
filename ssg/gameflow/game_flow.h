/// Application game-flow state machine.

#pragma once

#include <memory>

#include "sys/input.h"

struct GameContext;

namespace gameflow {

class GameFlow {
public:
  explicit GameFlow(GameContext &context);
  ~GameFlow();
  GameFlow(const GameFlow &) = delete;
  GameFlow(GameFlow &&) = delete;
  GameFlow &operator=(const GameFlow &) = delete;
  GameFlow &operator=(GameFlow &&) = delete;

  [[nodiscard]] bool Start();
  [[nodiscard]] bool Tick(InputBits input, InputBits system_input);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace gameflow
