///
/// EndingManager - Centralized ending cinematic state and operations
///

#pragma once

#include "ending.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "core/loader.h"
#include "platform/text_backend.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

struct EndingManager {
  // === Nested types (formerly static types in ENDING.cpp) ===

  struct GrpInfo {
    uint32_t timer = 0;
    uint32_t fadein = 0;
    uint32_t fadeout = 0;
    EndingGrp *target = nullptr;
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
    std::string_view Text[10];
    int NumText = 0;
    TEXTRENDER_RECT_ID Rect = {};

    // Contains all text from [Text], concatenated with '\n'.
    std::string TextStr;

    void Blank() {
      NumText = 0;
      TextStr.clear();
    }

    void Render(WINDOW_POINT topleft);
  };

  // === Data members ===

  GrpInfo grp_info;
  StTask stf_task;
  Text text;
  uint16_t flash_state = 0;

  static constexpr std::array<PIXEL_LTRB, 7> staff_label = {{
      {0, 0, 160, 24},
      {0, 24, 104, 48},
      {0, 48, 160, 72},
      {0, 72, 232, 96},
      {0, 96, 168, 120},
      {0, 144, 104, 168},
      {0, (480 - 32), (9 * 32), 480},
  }};

  static constexpr std::array<PIXEL_LTRB, 7> staff_member = {{
      {0, 168, 72, 192},
      {96, 168, 168, 192},
      {192, 168, 264, 192},
      {288, 168, 360, 192},
      {0, 192, 144, 216},
      {168, 192, 320, 216},
      {0, 216, 336, 264},
  }};

  // === Public methods ===

  bool Init();
  void Proc(bool &);
  void Draw();

private:
  // Internal helpers
  void UpdateGrpInfo();
  void UpdateStfInfo();
  void DrawGrpInfo();
  void DrawStfInfo();
  void DrawFadeInfo();
  void SCLDecode();
};

extern EndingManager Ending;

// === Backward-compatible inline wrapper (GameMain state machine entry point, must be kept) ===
inline bool EndingInit() { return Ending.Init(); }
inline void EndingProc(bool &q) { Ending.Proc(q); }
inline void EndingDraw() { Ending.Draw(); }
