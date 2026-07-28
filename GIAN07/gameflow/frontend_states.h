/// Title-side game-flow states.

#pragma once

#include <cstdint>
#include <optional>

#include "flow_types.h"

#include "effect/lens.h"
#include "gfx/coords.h"
#include "gfx/text.h"

struct GameContext;

namespace gameflow {

class ProjectState {
public:
  [[nodiscard]] bool Enter(GameContext &context);
  [[nodiscard]] FlowEvent Update(GameContext &context, const FrameInput &frame);

private:
  std::optional<LensInfo> lens_;
  uint16_t timer_ = 0;
};

class TitleState {
public:
  [[nodiscard]] bool Enter(GameContext &context, INPUT_BITS initial_input,
                           bool change_music);
  [[nodiscard]] FlowEvent Update(GameContext &context, const FrameInput &frame);

private:
  void InitVersion();
  void DrawVersion(PIXEL_COORD top) const;

  TEXTRENDER_RECT_ID version_rect_{};
  WINDOW_COORD version_left_ = 0;
  uint16_t demo_timer_ = 0;
};

class WeaponSelectState {
public:
  [[nodiscard]] bool Enter(GameContext &context, bool extra_stage);
  [[nodiscard]] FlowEvent Update(GameContext &context, const FrameInput &frame);

private:
  int count_ = 0;
  int angle_ = 0;
  int speed_ = 0;
  uint8_t key_wait_ = 0;
};

class BulletGalleryState {
public:
  [[nodiscard]] bool Enter(GameContext &context);
  [[nodiscard]] FlowEvent Update(GameContext &context, const FrameInput &frame);
};

} // namespace gameflow
