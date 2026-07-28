///
/// MenuTree — Unified tree-based menu item implementations
///

#include "menu_tree.h"

namespace menu {

// ---------------------------------------------------------------------------
// IMenuNode defaults
// ---------------------------------------------------------------------------

bool IMenuNode::OnAction(MenuController & /*ctrl*/) { return true; }

void IMenuNode::OnAdjust(MenuController & /*ctrl*/, int /*delta*/) {}

// ---------------------------------------------------------------------------
// EntryNode
// ---------------------------------------------------------------------------

EntryNode::EntryNode(std::string_view title, std::string_view help,
                     std::vector<std::unique_ptr<IMenuNode>> children)
    : IMenuNode(title, help) {
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
    for (auto &c : children_) {
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

ToggleNode::ToggleNode(std::string_view title, std::string_view help,
                       std::reference_wrapper<bool> ref, ChangeFn on_change)
    : IMenuNode(title, help), ref_(ref), on_change_(std::move(on_change)) {}

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
  return index < labels_.size() ? labels_[index] : std::string{};
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

ActionNode::ActionNode(std::string_view title, std::string_view help,
                       ActionFn action, AdjustFn adjust_fn)
    : IMenuNode(title, help), action_(std::move(action)),
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
  if (selected < scroll) {
    scroll = selected;
  }
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

ListNode::ListNode(std::string_view title, std::string_view help,
                   SizeFn size_fn, GenFn gen_fn, HandleFn handle_fn,
                   int init_sel, bool disable_value)
    : IMenuNode(title, help), current_idx_(init_sel),
      disable_value_(disable_value), handle_fn_(std::move(handle_fn)) {
  auto n = size_fn();
  list_view_.titles.reserve(n);
  for (size_t i = 0; i < n; i++) {
    list_view_.titles.push_back(gen_fn(i));
  }
  if (current_idx_ < 0 || static_cast<size_t>(current_idx_) >= n) {
    current_idx_ = 0;
  }
  list_view_.on_confirm = [this](size_t i) {
    bool stay = handle_fn_(i);
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
  if (idx < list_view_.titles.size()) {
    return list_view_.titles[idx];
  }
  return {};
}

int ListNode::CurrentIndex() const {
  return selection_fn_ ? selection_fn_() : current_idx_;
}

} // namespace menu
