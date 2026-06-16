/*
 *   EndingManager — centralized ending cinematic state and operations
 */

#pragma once

#include "ENDING.h"
#include "LOADER.h"
#include "game/coords.h"
#include "game/graphics.h"
#include "platform/text_backend.h"
#include <array>
#include <cstdint>
#include <string>

struct EndingManager {
  // === ネスト型（旧 ENDING.cpp ファイル静的型） ===

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
    Narrow::string_view Text[10];
    int NumText = 0;
    TEXTRENDER_RECT_ID Rect = {};

    // Contains all text from [Text], concatenated with '\n'.
    Narrow::string TextStr;

    void Blank() {
      NumText = 0;
      TextStr.clear();
    }

    void Render(WINDOW_POINT topleft);
  };

  // === データメンバー ===

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
      {0, 168, 72, 192},    {96, 168, 168, 192},
      {192, 168, 264, 192}, {288, 168, 360, 192},
      {0, 192, 144, 216},   {168, 192, 320, 216},
      {0, 216, 336, 264},
  }};

  // === 公開メソッド ===

  bool Init();
  void Proc(bool &);
  void Draw();

private:
  // 内部ヘルパー
  static void SetFixedColors(PALETTE &pal);
  static void FadeoutPaletteGrp(PALETTE &Dest, const PALETTE &Src, uint8_t a);
  static void FadeoutPaletteStf(PALETTE &Dest, const PALETTE &Src, uint8_t a);
  static void FlashPaletteGrp(PALETTE &dest, const PALETTE &pal, uint16_t a);

  void UpdateGrpInfo();
  void UpdateStfInfo();
  void DrawGrpInfo();
  void DrawStfInfo();
  void DrawFadeInfo();
  void SCLDecode();
};

extern EndingManager Ending;

// === 後方互換 inline wrapper (GameMain 状态机入口点，必须保留) ===
inline bool EndingInit() { return Ending.Init(); }
inline void EndingProc(bool &q) { Ending.Proc(q); }
inline void EndingDraw() { Ending.Draw(); }
