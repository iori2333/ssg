///
/// MenuController — Menu rendering, input handling, and stack navigation
///

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "menu_tree.h"

#include "gfx/coords.h"
#include "gfx/text.h"
#include "sys/input.h"

namespace menu {

inline constexpr auto kMenuItemH = 16;
inline constexpr auto kMenuItemPadX = 8;
inline constexpr auto kMenuDepthMax = 10;
inline constexpr auto kMenuKeyWait = 8;
inline constexpr auto kMenuTitleValueGap = 4;

constexpr FONT_ID kMenuFont = FONT_ID::SMALL;

enum class MenuColor {
  kActiveText,
  kActiveHighlight,
  kDisabledText,
  kDisabledHighlight,
};

struct MenuPage {
  std::string_view title;
  std::vector<IMenuNode *> items;
  int selected = 0;
  int scroll = 0;
};

class MenuController {
public:
  MenuController() = default;

  void Init(int window_width);
  void Open(WINDOW_POINT topleft, int select);
  void Navigate(IMenuNode &root_node, int initial_select = 0);

  void Tick(INPUT_BITS key);
  void Draw();

  bool Active() const { return !stack_.empty(); }
  int Depth() const;
  int Selection() const;
  int ClosedSelection() const { return closed_selection_; }
  INPUT_BITS LastKey() const { return last_key_; }
  std::string_view GetTitle() const;
  std::string_view GetCurrentHelp() const;
  int WindowY() const { return y_; }
  int WindowX() const { return x_; }
  int WindowWidth() const { return w_; }

  void SetY(int new_y) { y_ = new_y; }
  void AdjustYForTallMenu(int baseline_y, int max_visible);
  void SetLastKey(INPUT_BITS key) { last_key_ = key; }
  void SetRootCancelEnabled(bool enabled) { root_cancel_enabled_ = enabled; }

  void ActivateListView(ListView &view);
  void DeactivateListView();
  bool InListView() const { return active_list_ != nullptr; }

  // Pushes a page built from entry node's children plus auto Exit item.
  void PushPage(EntryNode &entry);
  void PopPage();
  void Close();

private:
  struct RenderSlot {
    TEXTRENDER_RECT_ID trr;
    std::string cache_key;
  };

  static constexpr auto kDefaultExitTitle = "Exit";
  static constexpr auto kDefaultExitHelp = "一つ前のメニューにもどります";

  void ProcessInput(INPUT_BITS key);
  void ProcessListInput(INPUT_BITS key);
  void BuildPageFromEntry(EntryNode &entry);
  void ResetNavigation(int initial_select);
  void InvalidateAllSlots();
  void RegisterTRRs();
  void RenderPage();
  void RenderList();

  static void DrawTitle(TEXTRENDER_SESSION &s, std::string_view title,
                        int rect_w);
  static void DrawItem(TEXTRENDER_SESSION &s, std::string_view title,
                       std::string_view value, int window_w, bool selected,
                       bool enabled, bool highlighted, bool centered);

  std::vector<MenuPage> stack_;
  std::vector<std::unique_ptr<IMenuNode>> exit_nodes_;
  std::vector<RenderSlot> slots_;
  IMenuNode *root_node_ = nullptr;
  ListView *active_list_ = nullptr;

  int x_ = 0;
  int y_ = 0;
  int w_ = 0;
  int closed_selection_ = 0;

  uint32_t frame_count_ = 0;
  INPUT_BITS last_key_ = 0;
  uint8_t key_wait_ = 0;
  uint8_t fast_repeat_wait_ = kMenuKeyWait;
  bool first_wait_ = false;
  bool root_cancel_enabled_ = true;
};

} // namespace menu
