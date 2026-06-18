///
/// ScrollMenu - Vertical scroll menu
///

#include "scroll_menu.h"

#include <algorithm>
#include <cassert>
#include <utility>

ScrollMenu::ScrollMenu(MenuLabel &title, ListSizeFn list_size,
                       GenerateFn generate, HandleFn handle, size_t max_visible)
    : menu_(), max_visible_(max_visible), list_size_(std::move(list_size)),
      generate_(std::move(generate)), handle_(std::move(handle)) {
  assert(max_visible_ <= items_.size());

  // Bind [menu_]'s item pointers to [items_].
  menu_.Title = &title;
  menu_.SetItems = [](MenuController &, bool) {};
  menu_.NumItems = 0;
  for (size_t i = 0; i < max_visible_; i++) {
    menu_.ItemPtr[i] = &items_[i];
  }
}

void ScrollMenu::Init(MenuController &ctrl, size_t sel,
                      MenuController *return_to) {
  assert(sel < list_size_());
  ctrl_ = &ctrl;
  return_to_ = return_to;
  sel_ = sel;
  menu_.NumItems = static_cast<uint8_t>((std::min)(list_size_(), max_visible_));
  Scroll();
}

void ScrollMenu::Scroll() {
  const auto total = list_size_();
  const auto visible = menu_.NumItems;
  const auto visible_half = (menu_.NumItems / 2);
  size_t generated_i =
      ((std::cmp_less(sel_, visible_half)) ? 0
       : (sel_ >= (total - visible_half))  ? (total - visible)
                                           : (sel_ - visible_half));
  for (auto item_i = decltype(visible){0}; item_i < visible; item_i++) {
    // Bind each item's callback to this instance's [Fn].
    items_[item_i].CallBackFn = [this](MenuController &c, INPUT_BITS k) {
      return Fn(c, k);
    };
    generate_(items_[item_i], generated_i, sel_);
    if (generated_i == sel_) {
      ctrl_->SetCurrentSelection(static_cast<uint8_t>(item_i));
    }
    generated_i++;
  }
}

bool ScrollMenu::Fn(MenuController &ctrl, INPUT_BITS key) {
  if (key == KEY_UP) {
    if (sel_ == 0) {
      sel_ = list_size_();
    }
    sel_--;
    Scroll();
  } else if (key == KEY_DOWN) {
    sel_++;
    if (sel_ >= list_size_()) {
      sel_ = 0;
    }
    Scroll();
  } else if ((key == KEY_BOMB) || (key == KEY_ESC)) {
    if (ctrl_->Depth() > 0) {
      ctrl_->PopLevel();
    } else {
      ctrl_->Close();
    }
    if (return_to_ != nullptr) {
      return_to_->SetLastKey(key);
    }
    return false;
  }
  return handle_(ctrl, key, sel_);
}
