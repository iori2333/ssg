///
/// EndingScene - ending cinematic UI state and operations
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "gfx/text_ttf.h"
#include "stage/scene_program.h"

class MusicPlayer;

namespace audio {
class AudioSystem;
}

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
    int timer = 0;
    int fadein = 0;
    int fadeout = 0;
    std::size_t picture_id = 0;
    int alpha = 0;
    int x = 0, y = 0;
    bool bWantDisp = false;
  };

  struct StTask {
    int timer = 0;
    int fadein = 0;
    int fadeout = 0;
    std::array<std::size_t, 10> StfID{};
    std::size_t TitleID = 0;
    std::size_t NumStf = 0;
    int alpha = 0;
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
  int flash_state = 0;

  static constexpr std::array<PixelLtrb, 7> staff_label = {{
      PixelLtrb{0, 0, 160, 24},
      PixelLtrb{0, 24, 104, 48},
      PixelLtrb{0, 48, 160, 72},
      PixelLtrb{0, 72, 232, 96},
      PixelLtrb{0, 96, 168, 120},
      PixelLtrb{0, 144, 104, 168},
      PixelLtrb{0, (480 - 32), (9 * 32), 480},
  }};

  static constexpr std::array<PixelLtrb, 7> staff_member = {{
      PixelLtrb{0, 168, 72, 192},
      PixelLtrb{96, 168, 168, 192},
      PixelLtrb{192, 168, 264, 192},
      PixelLtrb{288, 168, 360, 192},
      PixelLtrb{0, 192, 144, 216},
      PixelLtrb{168, 192, 320, 216},
      PixelLtrb{0, 216, 336, 264},
  }};

  void Draw();
  stage::SceneRunner scene_;

  // Internal helpers
  void UpdateGrpInfo();
  void UpdateStfInfo();
  void DrawGrpInfo();
  void DrawStfInfo();
  void DrawFadeInfo() const;
  [[nodiscard]] bool SCLDecode();

  data::GraphicsLoader &graphics_;
  stage::StageLoader &stage_loader_;
  MusicPlayer &music_;
  i18n::Localization &localization_;
  audio::AudioSystem &audio_;
};
