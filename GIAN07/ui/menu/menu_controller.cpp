///
/// MenuController — Menu rendering, input handling, and stack navigation
///

#include <algorithm>
#include <format>
#include <string_view>
#include <utility>

#include "menu_controller.h"

#include "audio/snd.h"
#include "gfx/graphics_backend.h"
#include "platform/text_backend.h"

namespace menu {

bool EntryNode::OnAction(MenuController &ctrl) {
  ctrl.PushPage(*this);
  return true;
}

bool ListNode::OnAction(MenuController &ctrl) {
  list_view_.title = title_;
  const auto current = CurrentIndex();
  list_view_.selected =
      current >= 0 && current < static_cast<int>(list_view_.titles.size())
          ? current
          : 0;
  ctrl.ActivateListView(list_view_);
  return true;
}

// ---------------------------------------------------------------------------
// MenuController
// ---------------------------------------------------------------------------

void MenuController::Init(int window_width) {
  w_ = window_width;
  RegisterTRRs();
}

void MenuController::Open(WINDOW_POINT topleft, int select,
                          INPUT_BITS initial_input) {
  ResetNavigation(select);

  x_ = topleft.x;
  y_ = topleft.y;

  frame_count_ = 0;
  closed_selection_ = 0;

  last_key_ = initial_input;
  key_wait_ = kMenuKeyWait;
  fast_repeat_wait_ = kMenuKeyWait;
  first_wait_ = true;

  InvalidateAllSlots();
}

void MenuController::InvalidateAllSlots() {
  for (auto &slot : slots_) {
    slot.cache_key.clear();
  }
}

void MenuController::Navigate(IMenuNode &root_node, int initial_select) {
  root_node_ = &root_node;
  ResetNavigation(initial_select);
}

void MenuController::ResetNavigation(int initial_select) {
  stack_.clear();
  exit_nodes_.clear();
  active_list_ = nullptr;

  auto *entry = dynamic_cast<EntryNode *>(root_node_);
  if (entry == nullptr) {
    return;
  }
  entry->OnPageEnter();

  MenuPage page;
  auto children = entry->Children();
  page.items.assign(children.begin(), children.end());
  page.title = root_node_->Title();
  page.selected = initial_select;
  stack_.push_back(std::move(page));

  InvalidateAllSlots();

  auto &p = stack_.back();
  if (p.selected >= static_cast<int>(p.items.size())) {
    p.selected = 0;
  }
  if (!p.items.empty() && !p.items[p.selected]->Enabled()) {
    int n = static_cast<int>(p.items.size());
    for (int i = 0; i < n; i++) {
      p.selected = (p.selected + 1) % n;
      if (p.items[p.selected]->Enabled()) {
        break;
      }
    }
  }
}

void MenuController::Tick(INPUT_BITS key) {
  if (stack_.empty()) {
    return;
  }

  if (InListView()) {
    ProcessListInput(key);
  } else {
    ProcessInput(key);
  }
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------

void MenuController::ProcessInput(INPUT_BITS key) {
  auto &page = stack_.back();

  if (first_wait_) {
    if (key != 0) {
      return;
    }
    first_wait_ = false;
  }

  if (key == 0 && key_wait_ != 0) {
    last_key_ = 0;
    key_wait_ = 0;
    return;
  }

  while (page.selected < static_cast<int>(page.items.size()) &&
         !page.items[page.selected]->Enabled()) {
    page.selected = (page.selected + 1) % static_cast<int>(page.items.size());
  }
  if (page.items.empty()) {
    return;
  }

  auto *node = page.items[page.selected];

  if (key_wait_ != 0) {
    key_wait_--;
    if (key_wait_ == 0) {
      last_key_ = 0;
    }
    return;
  }
  if (node->FastRepeat() && Input_OptionKeyDelta(last_key_) != 0) {
    key_wait_ = fast_repeat_wait_;
    fast_repeat_wait_ = (std::max)(fast_repeat_wait_ - 2, 0);
    if (key_wait_ == 0) {
      last_key_ = 0;
    }
    return;
  }
  if (last_key_ == KEY_UP || last_key_ == KEY_DOWN || last_key_ == KEY_LEFT ||
      last_key_ == KEY_RIGHT) {
    key_wait_ = kMenuKeyWait;
    return;
  }
  if (Input_IsOK(last_key_) || Input_IsCancel(last_key_)) {
    if (key == last_key_) {
      return;
    }
  } else {
    key_wait_ = 0;
  }

  last_key_ = key;

  if (key == KEY_UP || key == KEY_DOWN) {
    int dir = (key == KEY_UP) ? -1 : 1;
    int n = static_cast<int>(page.items.size());
    int cur = page.selected;
    do {
      cur = (cur + n + dir) % n;
    } while (!page.items[cur]->Enabled());
    page.selected = cur;

    if (page.selected < page.scroll) {
      page.scroll = page.selected;
    } else if (page.selected >= page.scroll + kMaxVisibleItems) {
      page.scroll = page.selected - kMaxVisibleItems + 1;
    }
    Snd_SEPlay(SfxId::Select);
    return;
  }

  if (Input_IsCancel(key)) {
    Snd_SEPlay(SfxId::Cancel);
  } else if (Input_OptionKeyDelta(key) != 0) {
    Snd_SEPlay(SfxId::Select);
  } else if (key == 0) {
    fast_repeat_wait_ = kMenuKeyWait;
  }

  if (auto delta = Input_OptionKeyDelta(key)) {
    if (!Input_IsOK(key)) {
      node->OnAdjust(*this, delta);
    }
  }
  if (Input_IsOK(key)) {
    if (node->Enabled()) {
      bool stay = node->OnAction(*this);
      if (!stay) {
        if (stack_.size() > 1) {
          PopPage();
        } else {
          closed_selection_ = page.selected;
          Close();
          last_key_ = 0;
        }
      }
    }
    return;
  }

  if (Input_IsCancel(key)) {
    if (stack_.size() > 1) {
      PopPage();
    } else if (root_cancel_enabled_) {
      closed_selection_ = -1;
      Close();
      last_key_ = 0;
    }
    return;
  }
}

// ---------------------------------------------------------------------------
// Page stack manipulation
// ---------------------------------------------------------------------------

void MenuController::PushPage(EntryNode &entry) {
  entry.OnPageEnter();
  BuildPageFromEntry(entry);
  InvalidateAllSlots();
}

void MenuController::PopPage() {
  if (stack_.size() > 1) {
    stack_.pop_back();
    InvalidateAllSlots();
  }
}

void MenuController::Close() {
  stack_.clear();
  active_list_ = nullptr;
}

void MenuController::BuildPageFromEntry(EntryNode &entry) {
  MenuPage page;
  page.title = entry.Title();
  auto children = entry.Children();
  page.items.reserve(children.size() + 1);
  for (auto *child : children) {
    page.items.push_back(child);
  }

  auto exit_action = [](MenuController &) { return false; };
  auto exit_node = std::make_unique<ActionNode>(
      kDefaultExitTitle, kDefaultExitHelp, std::move(exit_action));
  page.items.push_back(exit_node.get());
  exit_nodes_.push_back(std::move(exit_node));

  stack_.push_back(std::move(page));
}

void MenuController::RegisterTRRs() {
  slots_.clear();
  int needed = 1 + kMaxVisibleItems;
  for (int i = 0; i < needed; i++) {
    slots_.push_back({TextObj.Register({.w = w_, .h = kMenuItemH})});
  }
}

// ---------------------------------------------------------------------------
// Depth / selection accessors
// ---------------------------------------------------------------------------

int MenuController::Depth() const { return static_cast<int>(stack_.size()); }

int MenuController::Selection() const {
  if (stack_.empty()) {
    return closed_selection_;
  }
  return stack_.back().selected;
}

std::string_view MenuController::GetTitle() const {
  if (stack_.empty()) {
    return {};
  }
  return stack_.back().title;
}

std::string_view MenuController::GetCurrentHelp() const {
  if (stack_.empty()) {
    return {};
  }
  const auto &page = stack_.back();
  if (page.selected < 0 ||
      page.selected >= static_cast<int>(page.items.size())) {
    return {};
  }
  return page.items[page.selected]->Help();
}

// ---------------------------------------------------------------------------
// Tall menu adjustment
// ---------------------------------------------------------------------------

void MenuController::AdjustYForTallMenu(int baseline_y, int max_visible) {
  if (stack_.empty()) {
    return;
  }
  y_ = baseline_y;
  auto &page = stack_.back();
  if (static_cast<int>(page.items.size()) > max_visible) {
    y_ -= (static_cast<int>(page.items.size()) - max_visible) * kMenuItemH;
  }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void MenuController::Draw() {
  if (stack_.empty()) {
    return;
  }

  if (InListView()) {
    RenderList();
  } else {
    RenderPage();
  }
}

void MenuController::RenderPage() {
  auto &page = stack_.back();
  int visible = std::min(static_cast<int>(page.items.size()), kMaxVisibleItems);
  int box_alpha = (GrpGeom_FB() != nullptr) ? (64 + 32) : 128;

  int top = y_;

  GrpGeom->Lock();
  GrpGeom->SetAlphaNorm(box_alpha);

  GrpGeom->SetColor({0, 0, 0});
  GrpGeom->DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
  top += kMenuItemH;

  GrpGeom->SetColor({0, 0, 2});
  for (int i = page.scroll; i < page.scroll + visible; i++) {
    if (i == page.selected) {
      GrpGeom->SetAlphaNorm(128);
      GrpGeom->SetColor({5, 0, 0});
    }
    GrpGeom->DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
    top += kMenuItemH;
    if (i == page.selected) {
      GrpGeom->SetAlphaNorm(box_alpha);
      GrpGeom->SetColor({0, 0, 2});
    }
  }
  GrpGeom->Unlock();

  WINDOW_POINT pos = {x_, y_};
  std::string_view title_str = page.title;
  auto &title_slot = slots_[0];
  std::string title_key = std::format("T|{}", title_str);
  if (title_slot.cache_key != title_key) {
    title_slot.cache_key = title_key;
  }
  TextObj.Render(pos, title_slot.trr, title_slot.cache_key,
                 [&](TEXTRENDER_SESSION &s) { DrawTitle(s, title_str, w_); });

  pos.y += kMenuItemH;
  for (int i = page.scroll; i < page.scroll + visible; i++) {
    auto &node = *page.items[i];
    auto &slot = slots_[1 + (i - page.scroll)];
    bool selected = (i == page.selected);
    bool enabled = node.Enabled();
    bool highlighted = node.Highlighted();
    const auto value = node.Value();

    std::string key =
        std::format("{}|\x01[{}]|\x01{}{}{}{}", node.Title(), value,
                    selected ? 'S' : 'N', enabled ? 'E' : 'D',
                    highlighted ? 'H' : 'N', node.Centered() ? 'C' : 'N');

    if (slot.cache_key != key) {
      slot.cache_key = key;
    }

    TextObj.Render(pos, slot.trr, slot.cache_key, [&](TEXTRENDER_SESSION &s) {
      DrawItem(s, node.Title(), value, w_, selected, enabled, highlighted,
               node.Centered());
    });

    pos.y += kMenuItemH;
  }
}

// ---------------------------------------------------------------------------
// ListView activation
// ---------------------------------------------------------------------------

void MenuController::ActivateListView(ListView &view) {
  active_list_ = &view;
  InvalidateAllSlots();
}

void MenuController::DeactivateListView() {
  active_list_ = nullptr;
  InvalidateAllSlots();
}

// ---------------------------------------------------------------------------
// List-view input handling
// ---------------------------------------------------------------------------

void MenuController::ProcessListInput(INPUT_BITS key) {
  if (first_wait_) {
    if (key != 0) {
      return;
    }
    first_wait_ = false;
  }

  if (key == 0 && key_wait_ != 0) {
    last_key_ = 0;
    key_wait_ = 0;
    return;
  }

  if (key_wait_ != 0) {
    key_wait_--;
    if (key_wait_ == 0) {
      last_key_ = 0;
    }
    return;
  }

  if (last_key_ == KEY_UP || last_key_ == KEY_DOWN) {
    key_wait_ = kMenuKeyWait;
    return;
  }
  if (Input_IsOK(last_key_) || Input_IsCancel(last_key_)) {
    if (key == last_key_) {
      return;
    }
  } else {
    key_wait_ = 0;
  }

  last_key_ = key;

  if (key == KEY_UP || key == KEY_DOWN) {
    if (key == KEY_UP) {
      active_list_->MoveUp();
    } else {
      active_list_->MoveDown();
    }
    Snd_SEPlay(SfxId::Select);
    return;
  }

  if (Input_IsOK(key)) {
    bool stay = active_list_->Confirm();
    if (!stay) {
      Snd_SEPlay(SfxId::Cancel);
      DeactivateListView();
    } else {
      Snd_SEPlay(SfxId::Select);
    }
    return;
  }

  if (Input_IsCancel(key)) {
    Snd_SEPlay(SfxId::Cancel);
    DeactivateListView();
    return;
  }
}

// ---------------------------------------------------------------------------
// List-view rendering
// ---------------------------------------------------------------------------

void MenuController::RenderList() {
  auto *view = active_list_;
  int total = view->Total();
  int visible = (std::min)(total, kMaxVisibleItems);
  int box_alpha = (GrpGeom_FB() != nullptr) ? (64 + 32) : 128;

  int top = y_;

  GrpGeom->Lock();
  GrpGeom->SetAlphaNorm(box_alpha);

  GrpGeom->SetColor({0, 0, 0});
  GrpGeom->DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
  top += kMenuItemH;

  GrpGeom->SetColor({0, 0, 2});
  for (int i = view->scroll; i < view->scroll + visible; i++) {
    if (i == view->selected) {
      GrpGeom->SetAlphaNorm(128);
      GrpGeom->SetColor({5, 0, 0});
    }
    GrpGeom->DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
    top += kMenuItemH;
    if (i == view->selected) {
      GrpGeom->SetAlphaNorm(box_alpha);
      GrpGeom->SetColor({0, 0, 2});
    }
  }
  GrpGeom->Unlock();

  WINDOW_POINT pos = {x_, y_};
  auto &title_slot = slots_[0];
  std::string title_key = std::format("T|{}", view->title);
  if (title_slot.cache_key != title_key) {
    title_slot.cache_key = title_key;
  }
  TextObj.Render(pos, title_slot.trr, title_slot.cache_key,
                 [&](TEXTRENDER_SESSION &s) { DrawTitle(s, view->title, w_); });

  pos.y += kMenuItemH;
  for (int i = view->scroll; i < view->scroll + visible; i++) {
    auto &slot = slots_[1 + (i - view->scroll)];
    bool selected = (i == view->selected);

    auto n = static_cast<int>(view->titles.size());
    std::string_view item_title;
    if (i < n) {
      item_title = view->titles[i];
    } else {
      item_title = "Exit";
    }

    std::string key = std::format("{}|\x01{}|\x01{}{}{}{}", item_title, "",
                                  selected ? 'S' : 'N', 'E', 'N', 'N');

    if (slot.cache_key != key) {
      slot.cache_key = key;
    }

    TextObj.Render(pos, slot.trr, slot.cache_key, [&](TEXTRENDER_SESSION &s) {
      DrawItem(s, item_title, "", w_, selected, true, false, false);
    });

    pos.y += kMenuItemH;
  }
}

void MenuController::DrawTitle(TEXTRENDER_SESSION &s, std::string_view title,
                               int rect_w) {
  if (title.empty()) {
    return;
  }
  s.SetFont(kMenuFont);
  int cx = (rect_w - s.Extent(title).w) / 2;
  RGB white{255, 255, 255};
  s.Put({cx + 1, 0}, title, RGB{128, 128, 128});
  s.Put({cx, 0}, title, white);
}

void MenuController::DrawItem(TEXTRENDER_SESSION &s, std::string_view title,
                              std::string_view value, int window_w,
                              bool selected, bool enabled, bool highlighted,
                              bool centered) {
  s.SetFont(kMenuFont);

  const RGB shadow = enabled ? RGB{128, 128, 128} : RGB{96, 96, 96};
  const RGB text = enabled
                       ? (highlighted ? RGB{255, 255, 70} : RGB{255, 255, 255})
                       : (highlighted ? RGB{192, 192, 70} : RGB{192, 192, 192});

  int value_right = window_w - kMenuItemPadX;
  int title_left = kMenuItemPadX;

  int title_avail = value_right - kMenuTitleValueGap - title_left;
  if (!value.empty()) {
    std::string bracketed = std::format("[{}]", value);
    int vw = s.Extent(bracketed).w;
    int vx = value_right - vw;
    title_avail = vx - kMenuTitleValueGap - title_left;

    s.Put({vx + 1, 0}, bracketed, shadow);
    s.Put({vx, 0}, bracketed, text);
  }

  std::string display_title(title);
  if (s.Extent(display_title).w > title_avail) {
    auto ell_w = s.Extent("...").w;
    while (!display_title.empty() &&
           s.Extent(display_title).w + ell_w > title_avail) {
      display_title.pop_back();
    }
    display_title += "...";
  }

  int title_x = centered ? TextLayoutXCenter(s, display_title) : title_left;
  s.Put({title_x + 1, 0}, display_title, shadow);
  s.Put({title_x, 0}, display_title, text);
}

} // namespace menu
