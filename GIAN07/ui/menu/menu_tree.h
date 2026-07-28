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
#include <type_traits>
#include <utility>
#include <vector>

namespace menu {
class MenuController;

inline constexpr auto kMaxVisibleItems = 20;

class IMenuNode {
public:
  using EnabledFn = std::function<bool()>;

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

  virtual bool Enabled() const {
    return enabled_ && (!enabled_fn_ || enabled_fn_());
  }
  virtual bool Highlighted() const { return false; }
  virtual bool FastRepeat() const { return false; }
  virtual bool Centered() const { return false; }

  void SetEnabled(bool e) { enabled_ = e; }
  void BindEnabled(EnabledFn fn) { enabled_fn_ = std::move(fn); }

  virtual bool OnAction(MenuController &ctrl);
  virtual void OnAdjust(MenuController &ctrl, int delta);

  virtual std::span<IMenuNode *const> Children() const { return {}; }

  virtual void OnPageEnter() {}

protected:
  std::string_view title_;
  std::string_view help_;
  bool enabled_ = true;
  EnabledFn enabled_fn_;
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
  using ChangeFn = std::function<void(bool)>;

  ToggleNode(std::string_view title, std::string_view help,
             std::reference_wrapper<bool> ref, ChangeFn on_change = {});

  std::string Value() const override { return ref_.get() ? "ON" : "OFF"; }

  bool OnAction(MenuController &ctrl) override;
  void OnAdjust(MenuController &ctrl, int delta) override;

private:
  std::reference_wrapper<bool> ref_;
  ChangeFn on_change_;
};

// ---------------------------------------------------------------------------
// ChoiceNode — fixed selection from N options, uses LEFT/RIGHT to adjust
// ---------------------------------------------------------------------------
class ChoiceNode : public IMenuNode {
public:
  using ChangeFn = std::function<void()>;

  template <typename T>
    requires(std::is_integral_v<T> || std::is_enum_v<T>)
  ChoiceNode(std::string_view title, std::string_view help, T &value,
             std::type_identity_t<T> min, std::type_identity_t<T> max,
             std::vector<std::string> labels, ChangeFn on_change = {})
      : IMenuNode(title, help), labels_(std::move(labels)),
        on_change_(std::move(on_change)) {
    const auto as_integer = [](T choice) -> int64_t {
      if constexpr (std::is_enum_v<T>) {
        return static_cast<int64_t>(std::to_underlying(choice));
      } else {
        return static_cast<int64_t>(choice);
      }
    };
    const auto min_value = as_integer(min);
    const auto max_value = as_integer(max);
    value_fn_ = [&value, min_value, as_integer] {
      return static_cast<size_t>(as_integer(value) - min_value);
    };
    adjust_fn_ = [&value, min_value, max_value, as_integer](int delta) {
      auto current = as_integer(value);
      if (delta < 0) {
        current = current <= min_value ? max_value : current - 1;
      } else {
        current = current >= max_value ? min_value : current + 1;
      }
      value = static_cast<T>(current);
    };
  }

  std::string Value() const override;

  void OnAdjust(MenuController &ctrl, int delta) override;
  bool FastRepeat() const override { return true; }

private:
  using IndexFn = std::function<size_t()>;
  using AdjustFn = std::function<void(int)>;

  IndexFn value_fn_;
  AdjustFn adjust_fn_;
  std::vector<std::string> labels_;
  ChangeFn on_change_;
};

// ---------------------------------------------------------------------------
// ActionNode — custom callback on OK
// ---------------------------------------------------------------------------
class ActionNode : public IMenuNode {
public:
  using ActionFn = std::function<bool(MenuController &ctrl)>;
  using AdjustFn = std::function<void(MenuController &ctrl, int delta)>;
  using ValueFn = std::function<std::string()>;

  ActionNode(std::string_view title, std::string_view help, ActionFn action,
             AdjustFn adjust_fn = {});

  std::string Value() const override {
    return value_fn_ ? value_fn_() : std::string{};
  }

  bool OnAction(MenuController &ctrl) override;
  void OnAdjust(MenuController &ctrl, int delta) override;

  void BindValue(ValueFn fn) { value_fn_ = std::move(fn); }

private:
  ValueFn value_fn_;
  ActionFn action_;
  AdjustFn adjust_fn_;
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
  using ConfirmFn = std::function<bool(size_t)>;

  std::string_view title;
  std::vector<std::string> titles;
  ConfirmFn on_confirm;
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
  using SelectionFn = std::function<int()>;

  ListNode(std::string_view title, std::string_view help, SizeFn size_fn,
           GenFn gen_fn, HandleFn handle_fn, int init_sel = -1,
           bool disable_value = false);

  std::string Value() const override;

  bool OnAction(MenuController &ctrl) override;

  void BindSelection(SelectionFn fn) { selection_fn_ = std::move(fn); }

  ListView &GetListView() { return list_view_; }

private:
  [[nodiscard]] int CurrentIndex() const;

  int current_idx_ = -1;
  bool disable_value_ = false;
  HandleFn handle_fn_;
  SelectionFn selection_fn_;
  ListView list_view_;
};

// ---------------------------------------------------------------------------
} // namespace menu
