/*                                                                           */
/*   menu_renderer.h   メニューウィンドウ描画                                 */
/*                                                                           */
/*   描画処理を [window_sys.cpp] / [msg_window.cpp] から分離した。            */
/*                                                                           */

#pragma once

#include "window_sys.h" // MenuLabel, PIXEL_*

struct TEXTRENDER_SESSION;

// メニューラベル(タイトル / 項目)の描画 //
void MenuDrawLabel(TEXTRENDER_SESSION &s, const MenuLabel &label,
                   bool is_title);

// ウィンドウ枠を描画する //
void DrawWindowFrame(int x, int y, int w, int h);
