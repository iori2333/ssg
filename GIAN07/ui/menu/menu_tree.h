///
/// MenuTree — Unified tree-based menu item types
///

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace menu {
class MenuController;

inline constexpr auto kMaxVisibleItems = 20;

class IMenuNode {
public:
  IMenuNode(std::string_view title, std::string_view help)
      : title_(title), help_(help) {}
  virtual ~IMenuNode() = default;
  IMenuNode(const IMenuNode &) = delete;
  IMenuNode &operator=(const IMenuNode &) = delete;
  IMenuNode(IMenuNode &&) = default;
  IMenuNode &operator=(IMenuNode &&) = default;

  std::string_view Title() const { return title_; }
  virtual std::string Value() const = 0;
  std::string_view Help() const { return help_; }

  virtual bool Enabled() const { return enabled_; }
  virtual bool Highlighted() const { return false; }
  virtual bool FastRepeat() const { return false; }
  virtual bool Centered() const { return false; }

  void SetEnabled(bool e) { enabled_ = e; }
  void SetPoll(std::function<void()> fn) { poll_fn_ = std::move(fn); }

  virtual bool OnAction(MenuController &ctrl);
  virtual void OnAdjust(MenuController &ctrl, int delta);

  virtual void Poll() {
    if (poll_fn_) {
      poll_fn_();
    }
  }

  virtual std::span<IMenuNode *const> Children() const { return {}; }

  virtual void OnPageEnter() {}

protected:
  std::string_view title_;
  std::string_view help_;
  bool enabled_ = true;
  std::function<void()> poll_fn_;
};

// ---------------------------------------------------------------------------
// EntryNode — owns child nodes, navigates to submenu on OK
// ---------------------------------------------------------------------------
class EntryNode : public IMenuNode {
public:
  EntryNode(std::string_view title, std::string_view help,
            std::vector<std::unique_ptr<IMenuNode>> children);

  std::string Value() const override { return {}; }
  std::span<IMenuNode *const> Children() const override;

  bool OnAction(MenuController &ctrl) override;
  void OnPageEnter() override {}

  void ClearChildren();
  void AddChild(std::unique_ptr<IMenuNode> child);

private:
  std::vector<std::unique_ptr<IMenuNode>> children_;
  mutable std::vector<IMenuNode *> child_ptrs_;
};

// ---------------------------------------------------------------------------
// ToggleNode — boolean switch
// ---------------------------------------------------------------------------
class ToggleNode : public IMenuNode {
public:
  ToggleNode(std::string_view title, std::string_view help,
             std::reference_wrapper<bool> ref,
             std::function<void(bool)> on_change = {});

  std::string Value() const override { return ref_.get() ? "ON" : "OFF"; }

  bool OnAction(MenuController &ctrl) override;
  void OnAdjust(MenuController &ctrl, int delta) override;

private:
  std::reference_wrapper<bool> ref_;
  std::function<void(bool)> on_change_;
};

// ---------------------------------------------------------------------------
// ChoiceNode — fixed selection from N options, uses LEFT/RIGHT to adjust
// ---------------------------------------------------------------------------
class ChoiceNode : public IMenuNode {
public:
  ChoiceNode(std::string_view title, std::string_view help, uint8_t *value,
             uint8_t min, uint8_t max, std::vector<std::string> labels,
             std::function<void()> on_change = {});

  std::string Value() const override { return labels_[*value_]; }

  void OnAdjust(MenuController &ctrl, int delta) override;
  bool FastRepeat() const override { return true; }

private:
  uint8_t *value_;
  uint8_t min_;
  uint8_t max_;
  std::vector<std::string> labels_;
  std::function<void()> on_change_;
};

// ---------------------------------------------------------------------------
// ActionNode — custom callback on OK
// ---------------------------------------------------------------------------
class ActionNode : public IMenuNode {
public:
  using ActionFn = std::function<bool(MenuController &ctrl)>;

  ActionNode(std::string_view title, std::string_view help, ActionFn action,
             std::function<void(MenuController &, int)> adjust_fn = {});

  std::string Value() const override { return value_; }

  bool OnAction(MenuController &ctrl) override;
  void OnAdjust(MenuController &ctrl, int delta) override;

  void SetEnabled(bool e) { enabled_ = e; }
  void SetValue(std::string v) { value_ = std::move(v); }
  void SetActionFn(ActionFn fn) { action_ = std::move(fn); }

private:
  std::string value_;
  ActionFn action_;
  std::function<void(MenuController &, int)> adjust_fn_;
};

// ---------------------------------------------------------------------------
// SeparatorNode — non-selectable horizontal rule
// ---------------------------------------------------------------------------
class SeparatorNode : public IMenuNode {
public:
  explicit SeparatorNode(std::string_view label = "-------------------");

  std::string Value() const override { return {}; }
  bool Enabled() const override { return false; }
};

// ---------------------------------------------------------------------------
// ListView — item container + navigation, owned by ListNode
// ---------------------------------------------------------------------------
struct ListView {
  std::string_view title;
  std::vector<std::string> titles;
  std::function<bool(size_t)> on_confirm;
  int selected = 0;
  int scroll = 0;

  [[nodiscard]] int Total() const {
    return static_cast<int>(titles.size()) + 1;
  }
  void MoveUp();
  void MoveDown();
  bool Confirm() {
    auto n = static_cast<int>(titles.size());
    return selected < n ? on_confirm(static_cast<size_t>(selected)) : false;
  }
};

// ---------------------------------------------------------------------------
// ListNode — menu-node that opens an inline list view
// ---------------------------------------------------------------------------
class ListNode : public IMenuNode {
public:
  using SizeFn = std::function<size_t()>;
  using GenFn = std::function<std::string(size_t index)>;
  using HandleFn = std::function<bool(size_t index)>;

  ListNode(std::string_view title, std::string_view help, SizeFn size_fn,
           GenFn gen_fn, HandleFn handle_fn, int init_sel = -1,
           bool disable_value = false);

  std::string Value() const override;

  bool OnAction(MenuController &ctrl) override;

  ListView &GetListView() { return list_view_; }

private:
  int current_idx_ = -1;
  bool disable_value_ = false;
  HandleFn handle_fn_;
  ListView list_view_;
};

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------
void RingStepU8(uint8_t &var, int_fast8_t delta, uint8_t min, uint8_t max);

} // namespace menu
