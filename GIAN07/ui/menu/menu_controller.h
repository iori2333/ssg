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

constexpr FontId kMenuFont = FontId::Small;

enum class MenuColor {
  kActiveText,
  kActiveHighlight,
  kDisabledText,
  kDisabledHighlight,
};

struct MenuPage {
  IMenuNode *owner = nullptr;
  std::vector<IMenuNode *> items;
  int selected = 0;
  int scroll = 0;
};

class MenuController {
public:
  MenuController() = default;

  void Init(int window_width);
  void Open(WindowPoint topleft, int select, InputBits initial_input);
  void Navigate(IMenuNode &root_node, int initial_select = 0);

  void Tick(InputBits key);
  void Draw();

  [[nodiscard]] bool Active() const { return !stack_.empty(); }
  [[nodiscard]] int Depth() const;
  [[nodiscard]] int Selection() const;
  [[nodiscard]] int ClosedSelection() const { return closed_selection_; }
  [[nodiscard]] InputBits LastKey() const { return last_key_; }
  [[nodiscard]] std::string_view GetTitle() const;
  [[nodiscard]] std::string_view GetCurrentHelp() const;
  [[nodiscard]] int WindowY() const { return y_; }
  [[nodiscard]] int WindowX() const { return x_; }
  [[nodiscard]] int WindowWidth() const { return w_; }

  void SetY(int new_y) { y_ = new_y; }
  void AdjustYForTallMenu(int baseline_y, int max_visible);
  void SetLastKey(InputBits key) { last_key_ = key; }
  void SetRootCancelEnabled(bool enabled) { root_cancel_enabled_ = enabled; }
  void SetExitText(MenuText title, MenuText help) {
    exit_title_ = std::move(title);
    exit_help_ = std::move(help);
  }

  void ActivateListView(ListView &view);
  void DeactivateListView();
  [[nodiscard]] bool InListView() const { return active_list_ != nullptr; }

  // Pushes a page built from entry node's children plus auto Exit item.
  void PushPage(EntryNode &entry);
  void PopPage();
  void Close();

private:
  struct RenderSlot {
    TextRenderRectId trr;
    std::string cache_key;
  };

  void ProcessInput(InputBits key);
  void ProcessListInput(InputBits key);
  void BuildPageFromEntry(EntryNode &entry);
  void ResetNavigation(int initial_select);
  void InvalidateAllSlots();
  void RegisterTRRs();
  void RenderPage();
  void RenderList();

  static void DrawTitle(TextRenderSession &s, std::string_view title,
                        int rect_w, uint32_t marquee_frame);
  static void DrawItem(TextRenderSession &s, std::string_view title,
                       std::string_view value, int window_w, bool selected,
                       bool enabled, bool highlighted, bool centered,
                       uint32_t marquee_frame);

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
  InputBits last_key_ = 0;
  uint8_t key_wait_ = 0;
  uint8_t fast_repeat_wait_ = kMenuKeyWait;
  bool first_wait_ = false;
  bool root_cancel_enabled_ = true;
  MenuText exit_title_ = "Exit";
  MenuText exit_help_ = "一つ前のメニューにもどります";
};

} // namespace menu
