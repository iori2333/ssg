/*                                                                           */
/*   FontUty.h   フォントの処理いろいろ                                      */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

// 2000/07/22 : フォント追加に伴い、プログラムの一部を変更
// 2000/02/19 : フォントに関する処理の開発を始める

#include "game/text.h"

///// [ 関数 ] /////
void GrpPut16(int x, int y,
              const char *s); // 16x16 透過フォントで文字列出力(高速)
void GrpPut16c2(int x, int y,
                const char *s); // 上と同じだが、ｘ移動幅が１６
void GrpPutc(int x, int y,
             char c); // 16x16 透過フォントで文字出力(ｸﾘｯﾋﾟﾝｸﾞ有)
void GrpPut57(int x, int y, const char *s); // 05x07 べた貼りフォント
void GrpPut7B(int x, int y, const char *s); // 07x11 音楽室用フォント
void GrpPutScore(int x, int y,
                 const char *s); // 得点アイテムのスコアを描画

void GrpPutMidNum(int x, int y, int n); // MIDI 用フォントを描画する

// 5-pixel variable-width font in [SURFACE_ID::SYSTEM]. Supports A-Z.
// ------------------------------------------------------------------

constexpr PIXEL_COORD GrpExtent5(const char c) {
  if ((c < 'A') || (c > 'Z')) {
    assert(!"Character not supported in 5-pixel system font");
    return 0;
  }
  switch (c) {
  case 'I':
    return 3;
    break;
  case 'M':
  case 'T':
  case 'V':
  case 'W':
  case 'Y':
    return 5;
    break;
  default:
    return 4;
    break;
  }
}

constexpr PIXEL_SIZE GrpExtent5(std::string_view s) {
  PIXEL_SIZE ret = {.w = 0, .h = 5};
  for (const char c : s) {
    ret.w += (GrpExtent5(c) + 1);
  }
  ret.w = (std::max)(0, (ret.w - 1));
  return ret;
}

void GrpPut55(WINDOW_POINT topleft, std::string_view s);
// ------------------------------------------------------------------

// グラデーション付きフォントを描画する
PIXEL_SIZE DrawGrdFont(TEXTRENDER_SESSION &s, std::span<std::string_view> strs,
                       FONT_ID font, bool shadow,
                       uint8_t (*gradient_func)(PIXEL_COORD y));
