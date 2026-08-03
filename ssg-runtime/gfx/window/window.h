/// Game window event loop and persisted position.
#pragma once

#include <functional>
#include <optional>
#include <utility>

[[nodiscard]] std::optional<std::pair<int, int>> WindowPosition();

int WindowRun(const std::function<void()> &poll_input,
              const std::function<bool()> &run_frame);
