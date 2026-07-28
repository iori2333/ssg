/// Top-level application composition root.

#pragma once

#include "game_context.h"

#include "gameflow/game_flow.h"

class GameApplication {
public:
  GameApplication() : flow_(context_) {}

  [[nodiscard]] bool Start() { return flow_.Start(); }
  [[nodiscard]] bool Tick(INPUT_BITS input, INPUT_BITS system_input) {
    return flow_.Tick(input, system_input);
  }

  GameContext &Context() { return context_; }

private:
  GameContext context_;
  gameflow::GameFlow flow_;
};
