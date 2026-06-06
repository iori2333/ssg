/*                                                                           */
/*   Lens.h   レンズエフェクト                                               */
/*                                                                           */
/*                                                                           */

#pragma once

#include "game/coords.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

///// [構造体] /////

// レンズデータ定義用構造体 //
struct LensInfo {
  uint16_t r;                       // レンズの半径
  uint16_t Height;                  // レンズの直径
  std::unique_ptr<uint32_t[]> Data; // レンズ置換テーブル

  // Per-frame capture of the original back-buffer pixels under the lens.
  std::unique_ptr<std::byte[]> FOV;

  // GrpLock() 系関数 : レンズボールを描画する //
  void Draw(WINDOW_POINT center);
};

///// [ 関数 ] /////

// 半径:r  出っ張り:m  のレンズを作成 //
std::optional<LensInfo> GrpCreateLensBall(uint16_t r, uint16_t m);
