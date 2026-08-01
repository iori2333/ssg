///
/// EndingScene - ending cinematic UI state and operations
///

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "platform/text_backend.h"
#include "stage/scene_program.h"

class MusicPlayer;

namespace audio { class AudioSystem; }

namespace i18n {
class Localization;
}

namespace data {
class GraphicsLoader;
}

namespace stage {
class StageLoader;
}

class EndingScene {
public:
  EndingScene(data::GraphicsLoader &graphics, stage::StageLoader &stage_loader,
              MusicPlayer &music, i18n::Localization &localization,
              audio::AudioSystem &audio)
      : graphics_(graphics), stage_loader_(stage_loader), music_(music),
        localization_(localization), audio_(audio) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] bool Update(bool should_draw);

private:
  struct GrpInfo {
    uint32_t timer = 0;
    uint32_t fadein = 0;
    uint32_t fadeout = 0;
    uint8_t picture_id = 0;
    short alpha = 0;
    int x = 0, y = 0;
    bool bWantDisp = false;
  };

  struct StTask {
    uint32_t timer = 0;
    uint32_t fadein = 0;
    uint32_t fadeout = 0;
    uint8_t StfID[10] = {};
    uint8_t TitleID = 0;
    short NumStf = 0;
    short alpha = 0;
    int ox = 0, oy = 0;
    bool bWantDisp = false;
  };

  struct Text {
    std::vector<std::string_view> Text;
    TextRenderRectId Rect = {};

    // Contains all text from [Text], concatenated with '\n'.
    std::string TextStr;

    void Blank() {
      Text.clear();
      TextStr.clear();
    }

    void Render(WindowPoint topleft);
  };

  // === Data members ===

  GrpInfo grp_info;
  StTask stf_task;
  Text text;
  uint16_t flash_state = 0;

  static constexpr std::array<PixelLtrb, 7> staff_label = {{
      {0, 0, 160, 24},
      {0, 24, 104, 48},
      {0, 48, 160, 72},
      {0, 72, 232, 96},
      {0, 96, 168, 120},
      {0, 144, 104, 168},
      {0, (480 - 32), (9 * 32), 480},
  }};

  static constexpr std::array<PixelLtrb, 7> staff_member = {{
      {0, 168, 72, 192},
      {96, 168, 168, 192},
      {192, 168, 264, 192},
      {288, 168, 360, 192},
      {0, 192, 144, 216},
      {168, 192, 320, 216},
      {0, 216, 336, 264},
  }};

  void Draw();
  stage::SceneRunner scene_;

  // Internal helpers
  void UpdateGrpInfo();
  void UpdateStfInfo();
  void DrawGrpInfo();
  void DrawStfInfo();
  void DrawFadeInfo();
  [[nodiscard]] bool SCLDecode();

  data::GraphicsLoader &graphics_;
  stage::StageLoader &stage_loader_;
  MusicPlayer &music_;
  i18n::Localization &localization_;
  audio::AudioSystem &audio_;
};
