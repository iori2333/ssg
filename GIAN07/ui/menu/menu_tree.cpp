///
/// MenuTree — Unified tree-based menu item implementations
///

#include "menu_tree.h"

#include <algorithm>

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace menu {

// ---------------------------------------------------------------------------
// IMenuNode defaults
// ---------------------------------------------------------------------------

bool IMenuNode::OnAction(MenuController & /*ctrl*/) { return true; }

void IMenuNode::OnAdjust(MenuController & /*ctrl*/, int /*delta*/) {}

// ---------------------------------------------------------------------------
// EntryNode
// ---------------------------------------------------------------------------

EntryNode::EntryNode(MenuText title, MenuText help,
                     std::vector<std::unique_ptr<IMenuNode>> children)
    : IMenuNode(std::move(title), std::move(help)) {
  children_.reserve(children.size());
  child_ptrs_.reserve(children.size());
  for (auto &ch : children) {
    child_ptrs_.push_back(ch.get());
    children_.push_back(std::move(ch));
  }
}

std::span<IMenuNode *const> EntryNode::Children() const {
  if (child_ptrs_.size() != children_.size()) {
    child_ptrs_.clear();
    child_ptrs_.reserve(children_.size());
    for (const auto &c : children_) {
      child_ptrs_.push_back(c.get());
    }
  }
  return child_ptrs_;
}

void EntryNode::ClearChildren() {
  children_.clear();
  child_ptrs_.clear();
}

void EntryNode::AddChild(std::unique_ptr<IMenuNode> child) {
  child_ptrs_.push_back(child.get());
  children_.push_back(std::move(child));
}

// ---------------------------------------------------------------------------
// ToggleNode
// ---------------------------------------------------------------------------

ToggleNode::ToggleNode(MenuText title, MenuText help,
                       std::reference_wrapper<bool> ref, ChangeFn on_change)
    : IMenuNode(std::move(title), std::move(help)), ref_(ref),
      on_change_(std::move(on_change)) {}

std::string ToggleNode::Value() const {
  return std::string(ref_.get() ? on_.Get() : off_.Get());
}

bool ToggleNode::OnAction(MenuController & /*ctrl*/) {
  ref_.get() = !ref_.get();
  if (on_change_) {
    on_change_(ref_.get());
  }
  return true;
}

void ToggleNode::OnAdjust(MenuController & /*ctrl*/, int /*delta*/) {
  ref_.get() = !ref_.get();
  if (on_change_) {
    on_change_(ref_.get());
  }
}

// ---------------------------------------------------------------------------
// ChoiceNode
// ---------------------------------------------------------------------------

std::string ChoiceNode::Value() const {
  const auto index = value_fn_();
  return std::string(labels_.At(index));
}

void ChoiceNode::OnAdjust(MenuController & /*ctrl*/, int delta) {
  adjust_fn_(delta);
  if (on_change_) {
    on_change_();
  }
}

// ---------------------------------------------------------------------------
// ActionNode
// ---------------------------------------------------------------------------

ActionNode::ActionNode(MenuText title, MenuText help, ActionFn action,
                       AdjustFn adjust_fn)
    : IMenuNode(std::move(title), std::move(help)), action_(std::move(action)),
      adjust_fn_(std::move(adjust_fn)) {}

bool ActionNode::OnAction(MenuController &ctrl) { return action_(ctrl); }

void ActionNode::OnAdjust(MenuController &ctrl, int delta) {
  if (adjust_fn_) {
    adjust_fn_(ctrl, delta);
  }
}

// ---------------------------------------------------------------------------
// SeparatorNode
// ---------------------------------------------------------------------------

SeparatorNode::SeparatorNode(std::string_view label) : IMenuNode(label, "") {}

// ---------------------------------------------------------------------------
// ListView
// ---------------------------------------------------------------------------

void ListView::MoveUp() {
  auto total = static_cast<int>(titles.size()) + 1;
  if (selected == 0) {
    selected = total;
  }
  selected--;
  scroll = std::min(selected, scroll);
}

void ListView::MoveDown() {
  selected++;
  if (selected >= static_cast<int>(titles.size()) + 1) {
    selected = 0;
    scroll = 0;
  }
  if (selected >= scroll + kMaxVisibleItems) {
    scroll = selected - kMaxVisibleItems + 1;
  }
}

// ---------------------------------------------------------------------------
// ListNode
// ---------------------------------------------------------------------------

ListNode::ListNode(MenuText title, MenuText help, SizeFn size_fn, GenFn gen_fn,
                   HandleFn handle_fn, int init_sel, bool disable_value)
    : IMenuNode(std::move(title), std::move(help)), current_idx_(init_sel),
      disable_value_(disable_value), handle_fn_(std::move(handle_fn)),
      size_fn_(std::move(size_fn)), gen_fn_(std::move(gen_fn)) {
  const auto n = size_fn_();
  if (current_idx_ < 0 || std::cmp_greater_equal(current_idx_, n)) {
    current_idx_ = 0;
  }
  list_view_.on_confirm = [this](size_t i) {
    bool const stay = handle_fn_(i);
    if (!stay) {
      current_idx_ = static_cast<int>(i);
    }
    return stay;
  };
}

std::string ListNode::Value() const {
  const auto current = CurrentIndex();
  if (current < 0 || disable_value_) {
    return {};
  }
  auto idx = static_cast<size_t>(current);
  if (idx < size_fn_()) {
    return gen_fn_(idx);
  }
  return {};
}

int ListNode::CurrentIndex() const {
  return selection_fn_ ? selection_fn_() : current_idx_;
}

} // namespace menu
