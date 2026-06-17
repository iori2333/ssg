/*                                                                           */
/*   scroll_menu.h   垂直スクロールメニュー                                   */
/*                                                                           */
/*   元は [WINDOW_MENU_SCROLL] テンプレートだったが、テンプレート毎の         */
/*   static 状態を廃止し通常クラス化した。                                    */
/*                                                                           */

#pragma once

#include "window_sys.h" // WINDOW_CHOICE, WINDOW_MENU, WINDOW_LABEL, MenuController

#include <array>
#include <cstddef>
#include <functional>

class ScrollMenu {
public:
  using ListSizeFn = std::function<size_t()>;
  using GenerateFn =
      std::function<void(WINDOW_CHOICE &ret, size_t generated, size_t selected)>;
  using HandleFn =
      std::function<bool(MenuController &ctrl, INPUT_BITS key, size_t selected)>;

  ScrollMenu(WINDOW_LABEL &title, ListSizeFn list_size, GenerateFn generate,
             HandleFn handle, size_t max_visible = WINITEM_MAX);

  // [MenuController] の [Parent] にバインドする用。
  WINDOW_MENU &Menu() { return menu_; }

  // スクロールメニューを初期化し、最初の表示内容を生成する。
  void Init(MenuController &ctrl, size_t sel, MenuController *return_to);

private:
  void Scroll();
  bool Fn(MenuController &ctrl, INPUT_BITS key);

  std::array<WINDOW_CHOICE, WINITEM_MAX> items_{};
  WINDOW_MENU menu_;
  size_t max_visible_ = WINITEM_MAX;

  size_t sel_ = 0;
  MenuController *ctrl_ = nullptr;
  MenuController *return_to_ = nullptr;

  ListSizeFn list_size_;
  GenerateFn generate_;
  HandleFn handle_;
};
