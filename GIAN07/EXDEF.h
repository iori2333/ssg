/*                                                                           */
/*   ExDef.h   いろんなものの定義とか                                        */
/*                                                                           */
/*                                                                           */

#pragma once

#include <cstdint>

// 角度付き座標の管理用構造体 //
struct DegPoint {
  int x, y;  // 座標
  uint8_t d; // 角度
};
