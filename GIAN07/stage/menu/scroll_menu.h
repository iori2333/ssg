///
/// ScrollMenu - Vertical scroll menu
///

#pragma once

#include <array>
#include <cstddef>
#include <functional>

#include "stage/menu/window_sys.h"

class ScrollMenu {
public:
  using ListSizeFn = std::function<size_t()>;
  using GenerateFn =
      std::function<void(MenuItem &ret, size_t generated, size_t selected)>;
  using HandleFn = std::function<bool(MenuController &ctrl, INPUT_BITS key,
                                      size_t selected)>;

  ScrollMenu(MenuLabel &title, ListSizeFn list_size, GenerateFn generate,
             HandleFn handle, size_t max_visible = WINITEM_MAX);

  // For binding to [MenuController]'s [Parent].
  MenuDef &Menu() { return menu_; }

  // Initializes the scroll menu and generates the initial display content.
  void Init(MenuController &ctrl, size_t sel, MenuController *return_to);

private:
  void Scroll();
  bool Fn(MenuController &ctrl, INPUT_BITS key);

  std::array<MenuItem, WINITEM_MAX> items_{};
  MenuDef menu_;
  size_t max_visible_ = WINITEM_MAX;

  size_t sel_ = 0;
  MenuController *ctrl_ = nullptr;
  MenuController *return_to_ = nullptr;

  ListSizeFn list_size_;
  GenerateFn generate_;
  HandleFn handle_;
};
