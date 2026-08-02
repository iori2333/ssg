///
/// MenuController — Menu rendering, input handling, and stack navigation
///

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string_view>
#include <utility>

#include "menu_controller.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/text.h"
#include "platform/text_backend.h"
#include "platform/windows/text_gdi.h"
#include "sys/input.h"
#include "ui/menu/menu_tree.h"
#include "ui/text_marquee.h"

namespace menu {

bool EntryNode::OnAction(MenuController &ctrl) {
  ctrl.PushPage(*this);
  return true;
}

bool ListNode::OnAction(MenuController &ctrl) {
  list_view_.title = title_;
  const auto size = size_fn_();
  list_view_.titles.clear();
  list_view_.titles.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    list_view_.titles.push_back(gen_fn_(i));
  }
  const auto current = CurrentIndex();
  list_view_.selected =
      current >= 0 && std::cmp_less(current, list_view_.titles.size()) ? current
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

void MenuController::Open(WindowPoint topleft, int select,
                          InputBits initial_input) {
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
  page.owner = root_node_;
  page.selected = initial_select;
  stack_.push_back(std::move(page));

  InvalidateAllSlots();

  auto &p = stack_.back();
  if (std::cmp_greater_equal(p.selected, p.items.size())) {
    p.selected = 0;
  }
  if (!p.items.empty() && !p.items[p.selected]->Enabled()) {
    int const n = static_cast<int>(p.items.size());
    for (int i = 0; i < n; i++) {
      p.selected = (p.selected + 1) % n;
      if (p.items[p.selected]->Enabled()) {
        break;
      }
    }
  }
}

void MenuController::Tick(InputBits key) {
  if (stack_.empty()) {
    return;
  }

  if (InListView()) {
    ProcessListInput(key);
  } else {
    ProcessInput(key);
  }
  frame_count_++;
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------

void MenuController::ProcessInput(InputBits key) {
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

  while (std::cmp_less(page.selected, page.items.size()) &&
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
  if (node->FastRepeat() && InputOptionKeyDelta(last_key_) != 0) {
    key_wait_ = fast_repeat_wait_;
    fast_repeat_wait_ = (std::max)(fast_repeat_wait_ - 2, 0);
    if (key_wait_ == 0) {
      last_key_ = 0;
    }
    return;
  }
  if (last_key_ == KeyUp || last_key_ == KeyDown || last_key_ == KeyLeft ||
      last_key_ == KeyRight) {
    key_wait_ = kMenuKeyWait;
    return;
  }
  if (InputIsOk(last_key_) || InputIsCancel(last_key_)) {
    if (key == last_key_) {
      return;
    }
  } else {
    key_wait_ = 0;
  }

  last_key_ = key;

  if (key == KeyUp || key == KeyDown) {
    int const dir = (key == KeyUp) ? -1 : 1;
    int const n = static_cast<int>(page.items.size());
    int cur = page.selected;
    cur = (cur + n + dir) % n;
    while (!page.items[cur]->Enabled()) {
      cur = (cur + n + dir) % n;
    }
    page.selected = cur;
    frame_count_ = 0;

    if (page.selected < page.scroll) {
      page.scroll = page.selected;
    } else if (page.selected >= page.scroll + kMaxVisibleItems) {
      page.scroll = page.selected - kMaxVisibleItems + 1;
    }
    audio_.PlaySfx(SfxId::Select);
    return;
  }

  if (InputIsCancel(key)) {
    audio_.PlaySfx(SfxId::Cancel);
  } else if (InputOptionKeyDelta(key) != 0) {
    audio_.PlaySfx(SfxId::Select);
  } else if (key == 0) {
    fast_repeat_wait_ = kMenuKeyWait;
  }

  if (auto delta = InputOptionKeyDelta(key)) {
    if (!InputIsOk(key)) {
      frame_count_ = 0;
      node->OnAdjust(*this, delta);
    }
  }
  if (InputIsOk(key)) {
    if (node->Enabled()) {
      audio_.PlaySfx(SfxId::Select);
      frame_count_ = 0;
      bool const stay = node->OnAction(*this);
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

  if (InputIsCancel(key)) {
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
  frame_count_ = 0;
  InvalidateAllSlots();
}

void MenuController::PopPage() {
  if (stack_.size() > 1) {
    stack_.pop_back();
    frame_count_ = 0;
    InvalidateAllSlots();
  }
}

void MenuController::Close() {
  stack_.clear();
  active_list_ = nullptr;
}

void MenuController::BuildPageFromEntry(EntryNode &entry) {
  MenuPage page;
  page.owner = &entry;
  auto children = entry.Children();
  page.items.reserve(children.size() + 1);
  for (auto *child : children) {
    page.items.push_back(child);
  }

  auto exit_action = [](MenuController &) { return false; };
  auto exit_node = std::make_unique<ActionNode>(exit_title_, exit_help_,
                                                std::move(exit_action));
  page.items.push_back(exit_node.get());
  exit_nodes_.push_back(std::move(exit_node));

  stack_.push_back(std::move(page));
}

void MenuController::RegisterTRRs() {
  slots_.clear();
  int const needed = 1 + kMaxVisibleItems;
  for (int i = 0; i < needed; i++) {
    slots_.push_back(
        {.trr = TextRenderer().Register({.w = w_, .h = kMenuItemH})});
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
  const auto *owner = stack_.back().owner;
  return owner != nullptr ? owner->Title() : std::string_view{};
}

std::string_view MenuController::GetCurrentHelp() const {
  if (stack_.empty()) {
    return {};
  }
  const auto &page = stack_.back();
  if (page.selected < 0 ||
      std::cmp_greater_equal(page.selected, page.items.size())) {
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
  if (std::cmp_greater(page.items.size(), max_visible)) {
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
  int const visible =
      std::min(static_cast<int>(page.items.size()), kMaxVisibleItems);
  constexpr int box_alpha = 128;

  int top = y_;

  geometry::SetAlphaNorm(box_alpha);

  geometry::SetColor({0, 0, 0});
  geometry::DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
  top += kMenuItemH;

  geometry::SetColor({0, 0, 2});
  for (int i = page.scroll; i < page.scroll + visible; i++) {
    if (i == page.selected) {
      geometry::SetAlphaNorm(128);
      geometry::SetColor({5, 0, 0});
    }
    geometry::DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
    top += kMenuItemH;
    if (i == page.selected) {
      geometry::SetAlphaNorm(box_alpha);
      geometry::SetColor({0, 0, 2});
    }
  }

  WindowPoint pos = {x_, y_};
  const auto title_str =
      page.owner != nullptr ? page.owner->Title() : std::string_view{};
  auto &title_slot = slots_[0];
  std::string title_key = std::format("T|{}", title_str);
  title_key += std::format("|{}", frame_count_ / ui::kMarqueeStepFrames);
  if (title_slot.cache_key != title_key) {
    title_slot.cache_key = title_key;
  }
  TextRenderer().Render(
      pos, title_slot.trr, title_slot.cache_key,
      [&](TextRenderSession &s) { DrawTitle(s, title_str, w_, frame_count_); });

  pos.y += kMenuItemH;
  for (int i = page.scroll; i < page.scroll + visible; i++) {
    auto &node = *page.items[i];
    auto &slot = slots_[1 + (i - page.scroll)];
    bool selected = (i == page.selected);
    bool enabled = node.Enabled();
    bool highlighted = node.Highlighted();
    const auto value = node.Value();
    const auto marquee_frame = selected ? frame_count_ : 0;

    std::string const key = std::format(
        "{}|\x01[{}]|\x01{}{}{}{}|{}", node.Title(), value,
        selected ? 'S' : 'N', enabled ? 'E' : 'D', highlighted ? 'H' : 'N',
        node.Centered() ? 'C' : 'N', marquee_frame / ui::kMarqueeStepFrames);

    if (slot.cache_key != key) {
      slot.cache_key = key;
    }

    TextRenderer().Render(
        pos, slot.trr, slot.cache_key, [&](TextRenderSession &s) {
          DrawItem(s, node.Title(), value, w_, selected, enabled, highlighted,
                   node.Centered(), marquee_frame);
        });

    pos.y += kMenuItemH;
  }
}

// ---------------------------------------------------------------------------
// ListView activation
// ---------------------------------------------------------------------------

void MenuController::ActivateListView(ListView &view) {
  active_list_ = &view;
  frame_count_ = 0;
  InvalidateAllSlots();
}

void MenuController::DeactivateListView() {
  active_list_ = nullptr;
  frame_count_ = 0;
  InvalidateAllSlots();
}

// ---------------------------------------------------------------------------
// List-view input handling
// ---------------------------------------------------------------------------

void MenuController::ProcessListInput(InputBits key) {
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

  if (last_key_ == KeyUp || last_key_ == KeyDown) {
    key_wait_ = kMenuKeyWait;
    return;
  }
  if (InputIsOk(last_key_) || InputIsCancel(last_key_)) {
    if (key == last_key_) {
      return;
    }
  } else {
    key_wait_ = 0;
  }

  last_key_ = key;

  if (key == KeyUp || key == KeyDown) {
    if (key == KeyUp) {
      active_list_->MoveUp();
    } else {
      active_list_->MoveDown();
    }
    frame_count_ = 0;
    audio_.PlaySfx(SfxId::Select);
    return;
  }

  if (InputIsOk(key)) {
    bool const stay = active_list_->Confirm();
    if (!stay) {
      audio_.PlaySfx(SfxId::Cancel);
      DeactivateListView();
    } else {
      audio_.PlaySfx(SfxId::Select);
    }
    return;
  }

  if (InputIsCancel(key)) {
    audio_.PlaySfx(SfxId::Cancel);
    DeactivateListView();
    return;
  }
}

// ---------------------------------------------------------------------------
// List-view rendering
// ---------------------------------------------------------------------------

void MenuController::RenderList() {
  auto *view = active_list_;
  int const total = view->Total();
  int const visible = (std::min)(total, kMaxVisibleItems);
  constexpr int box_alpha = 128;

  int top = y_;

  geometry::SetAlphaNorm(box_alpha);

  geometry::SetColor({0, 0, 0});
  geometry::DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
  top += kMenuItemH;

  geometry::SetColor({0, 0, 2});
  for (int i = view->scroll; i < view->scroll + visible; i++) {
    if (i == view->selected) {
      geometry::SetAlphaNorm(128);
      geometry::SetColor({5, 0, 0});
    }
    geometry::DrawBoxA(x_, top, x_ + w_, top + kMenuItemH);
    top += kMenuItemH;
    if (i == view->selected) {
      geometry::SetAlphaNorm(box_alpha);
      geometry::SetColor({0, 0, 2});
    }
  }

  WindowPoint pos = {x_, y_};
  auto &title_slot = slots_[0];
  const auto title = view->title.Get();
  std::string const title_key =
      std::format("T|{}|{}", title, frame_count_ / ui::kMarqueeStepFrames);
  if (title_slot.cache_key != title_key) {
    title_slot.cache_key = title_key;
  }
  TextRenderer().Render(
      pos, title_slot.trr, title_slot.cache_key,
      [&](TextRenderSession &s) { DrawTitle(s, title, w_, frame_count_); });

  pos.y += kMenuItemH;
  for (int i = view->scroll; i < view->scroll + visible; i++) {
    auto &slot = slots_[1 + (i - view->scroll)];
    bool selected = (i == view->selected);

    auto n = static_cast<int>(view->titles.size());
    std::string_view item_title;
    if (i < n) {
      item_title = view->titles[i];
    } else {
      item_title = exit_title_.Get();
    }

    std::string key = std::format("{}|\x01{}|\x01{}{}{}{}", item_title, "",
                                  selected ? 'S' : 'N', 'E', 'N', 'N');
    const auto marquee_frame = selected ? frame_count_ : 0;
    key += std::format("|{}", marquee_frame / ui::kMarqueeStepFrames);

    if (slot.cache_key != key) {
      slot.cache_key = key;
    }

    TextRenderer().Render(pos, slot.trr, slot.cache_key,
                          [&](TextRenderSession &s) {
                            DrawItem(s, item_title, "", w_, selected, true,
                                     false, false, marquee_frame);
                          });

    pos.y += kMenuItemH;
  }
}

void MenuController::DrawTitle(TextRenderSession &s, std::string_view title,
                               int rect_w, uint32_t marquee_frame) {
  if (title.empty()) {
    return;
  }
  s.SetFont(kMenuFont);
  constexpr int kTitlePad = kMenuItemPadX;
  const int available_width = rect_w - kTitlePad * 2;
  const bool scroll = TextRenderSession::Extent(title).w > available_width;
  const auto display_title =
      ui::MarqueeWindow(s, title, available_width, marquee_frame);
  const int x = scroll
                    ? kTitlePad
                    : (rect_w - TextRenderSession::Extent(display_title).w) / 2;
  Rgb white{.r = 255, .g = 255, .b = 255};
  s.Put({.x = x + 1, .y = 0}, display_title, Rgb{.r = 128, .g = 128, .b = 128});
  s.Put({.x = x, .y = 0}, display_title, white);
}

void MenuController::DrawItem(TextRenderSession &s, std::string_view title,
                              std::string_view value, int window_w,
                              bool /*selected*/, bool enabled, bool highlighted,
                              bool centered, uint32_t marquee_frame) {
  s.SetFont(kMenuFont);

  const Rgb shadow = enabled ? Rgb{.r = 128, .g = 128, .b = 128}
                             : Rgb{.r = 96, .g = 96, .b = 96};
  Rgb text = Rgb{.r = 192, .g = 192, .b = 192};
  if (enabled) {
    text = highlighted ? Rgb{.r = 255, .g = 255, .b = 70}
                       : Rgb{.r = 255, .g = 255, .b = 255};
  } else if (highlighted) {
    text = Rgb{.r = 192, .g = 192, .b = 70};
  }

  int const value_right = window_w - kMenuItemPadX;
  int const title_left = kMenuItemPadX;

  const std::string display_title(title);
  const int title_width = TextRenderSession::Extent(display_title).w;
  const int title_x =
      centered ? TextLayoutXCenter(s, display_title) : title_left;

  if (!value.empty()) {
    const int value_left = display_title.empty()
                               ? title_left
                               : title_x + title_width + kMenuTitleValueGap;
    const std::string bracketed = std::format("[{}]", value);
    if (TextRenderSession::Extent(bracketed).w <= value_right - value_left) {
      const int value_x = value_right - TextRenderSession::Extent(bracketed).w;
      s.Put({.x = value_x + 1, .y = 0}, bracketed, shadow);
      s.Put({.x = value_x, .y = 0}, bracketed, text);
    } else {
      const int open_width = TextRenderSession::Extent("[").w;
      const int close_width = TextRenderSession::Extent("]").w;
      const int content_left = value_left + open_width;
      const int close_x = value_right - close_width;
      const auto display_value =
          ui::MarqueeWindow(s, value, close_x - content_left, marquee_frame);
      if (content_left <= close_x) {
        s.Put({.x = value_left + 1, .y = 0}, "[", shadow);
        s.Put({.x = value_left, .y = 0}, "[", text);
        s.Put({.x = content_left + 1, .y = 0}, display_value, shadow);
        s.Put({.x = content_left, .y = 0}, display_value, text);
        s.Put({.x = close_x + 1, .y = 0}, "]", shadow);
        s.Put({.x = close_x, .y = 0}, "]", text);
      }
    }
  } else {
    const int available_width = value_right - title_left;
    const bool scroll = title_width > available_width;
    const auto single_line =
        ui::MarqueeWindow(s, title, available_width, marquee_frame);
    const int single_x =
        centered && !scroll ? TextLayoutXCenter(s, single_line) : title_left;
    s.Put({.x = single_x + 1, .y = 0}, single_line, shadow);
    s.Put({.x = single_x, .y = 0}, single_line, text);
    return;
  }

  s.Put({.x = title_x + 1, .y = 0}, display_title, shadow);
  s.Put({.x = title_x, .y = 0}, display_title, text);
}

} // namespace menu
