///
/// WindowSys - Command window processing
///

#pragma once

#include <functional>
#include <utility>

#include "game/enum_flags.h"
#include "game/input.h"
#include "game/text.h"

// [Revision history]
// 2000/07/26 : Improved operability
//
// 2000/02/28 : Faster semi-transparent message window
// 2000/02/10 : PWINDOW -> WINDOWSYS
//            : In WINDOWSYS, the command window holds selection state on a stack.
// TODO: Moving windows and adding frames (deferred)
//
// 2000/01/31 : Development started

// Constants
// ---------

// Constants related to maximum counts
inline constexpr auto WINITEM_MAX = 20;  // Maximum number of items
inline constexpr auto WINDOW_DEPTH = 10; // Window depth
inline constexpr auto MSG_HEIGHT = 5;    // Message window height

// Command window state
inline constexpr auto CWIN_DEAD = 0x00;   // Not in use
inline constexpr auto CWIN_FREE = 0x01;   // Waiting for input
inline constexpr auto CWIN_OPEN = 0x02;   // Moving through items (forward)
inline constexpr auto CWIN_CLOSE = 0x03;  // Moving through items (back)
inline constexpr auto CWIN_NEXT = 0x04;   // Transitioning to next window
inline constexpr auto CWIN_BEFORE = 0x05; // Transitioning to previous window
inline constexpr auto CWIN_INIT = 0xff;   // Initializing

constexpr auto CWIN_FONT = FONT_ID::SMALL;

constexpr PIXEL_COORD CWIN_ITEM_LEFT = 8;
constexpr PIXEL_COORD CWIN_ITEM_H = 16;
constexpr PIXEL_COORD CWIN_MAX_H = ((WINITEM_MAX + 1) * CWIN_ITEM_H);

constexpr PIXEL_COORD FACE_W = 96;
constexpr PIXEL_COORD FACE_H = 96;

// Message window commands
inline constexpr auto MWCMD_SMALLFONT = 0x00;  // Use small font
inline constexpr auto MWCMD_NORMALFONT = 0x01; // Use normal font
inline constexpr auto MWCMD_LARGEFONT = 0x02;  // Use large font
inline constexpr auto MWCMD_NEWPAGE = 0x03;    // New page

// Message window state
inline constexpr auto MWIN_DEAD = 0x00;  // Not in use
inline constexpr auto MWIN_OPEN = 0x01;  // Opening
inline constexpr auto MWIN_CLOSE = 0x02; // Closing
inline constexpr auto MWIN_FREE = 0x03;  // Waiting

// Message window (face state)
inline constexpr auto MFACE_NONE = 0x00;  // Not displayed
inline constexpr auto MFACE_OPEN = 0x01;  // Opening
inline constexpr auto MFACE_CLOSE = 0x02; // Closing
inline constexpr auto MFACE_NEXT = 0x03;  // Next face
inline constexpr auto MFACE_WAIT = 0x04;  // Waiting

// Other constants
inline constexpr auto CWIN_KEYWAIT = 8;
// ---------

// [Structures]

enum class MenuFlags : uint8_t {
  HAS_BITFLAG_OPERATORS = 0,
  NONE = 0x00,

  // Shortens the key repeat times for option items.
  FAST_REPEAT = 0x01,

  // Horizontally centered text.
  CENTER = 0x02,

  // Cannot be selected.
  DISABLED = 0x04,

  // Rendered in the highlight color.
  HIGHLIGHT = 0x08,

  FORCE_RERENDER = 0x10,
};

struct MenuDef;
class MenuController;

// Shared data for menu titles and choices.
struct MenuLabel {
  const char *Title; // Pointer to title string (not the actual string!)

  MenuFlags Flags = MenuFlags::NONE;

  // Required for forcing the item to be re-rendered after a flag change.
  MenuFlags FlagsPrev = MenuFlags::FORCE_RERENDER;

  constexpr MenuLabel(const char *title = "",
                      MenuFlags flags = MenuFlags::NONE) noexcept
      : Title(title), Flags(flags) {}
};

// Sub-window information
struct MenuItem : public MenuLabel {
  // Callback function type
  using ActionFn = std::function<bool(MenuController &, INPUT_BITS)>;
  using AdjustFn = std::function<void(MenuController &, int_fast8_t)>;

  // Raw pointer aliases for constructor overloading disambiguation.
  using ActionFnPtr = bool (*)(MenuController &, INPUT_BITS);
  using AdjustFnPtr = void (*)(MenuController &, int_fast8_t);

  const char *Help; // Pointer to help string (not the actual string)

  // Special callback function (empty if unused)
  ActionFn CallBackFn;

  // Callback function for options (falls back on [CallBackFn] if empty)
  AdjustFn OptionFn;

  MenuDef *Submenu = nullptr;

  MenuItem(const char *title = "", const char *help = "",
           ActionFnPtr callback_fn = nullptr, MenuFlags flags = MenuFlags::NONE)
      : MenuLabel(title, flags), Help(help), CallBackFn(callback_fn) {
    if (!CallBackFn) {
      Flags |= MenuFlags::DISABLED;
    }
  }

  MenuItem(const char *title, const char *help, AdjustFnPtr option_fn,
           MenuFlags flags = MenuFlags::NONE)
      : MenuLabel(title, flags), Help(help), OptionFn(option_fn) {}

  MenuItem(const char *title, const char *help, MenuFlags flags)
      : MenuLabel(title, flags), Help(help) {}

  MenuItem(const char *title, const char *help, MenuDef &submenu)
      : MenuLabel(title), Help(help), Submenu(&submenu) {}

  void SetActive(bool active);
};

// Dynamic collection of menu items at a single hierarchy level.
struct MenuDef {
  using RefreshFn = std::function<void(MenuController &, bool tick)>;

  MenuLabel *Title = nullptr;
  MenuItem *ItemPtr[WINITEM_MAX] = {nullptr}; // Pointer to next item
  RefreshFn SetItems = [](MenuController &, bool) {};
  uint8_t NumItems = 0; // Number of items (<ITEM_MAX)

  MenuDef() = default;

  template <size_t N>
  MenuDef(
      std::span<MenuItem, N> children,
      RefreshFn set_items = [](MenuController &, bool) {},
      MenuLabel *title = nullptr) noexcept
      : Title(title), SetItems(std::move(set_items)), NumItems(N) {
    static_assert(N <= WINITEM_MAX);
    for (size_t i = 0; auto &item : children) {
      ItemPtr[i++] = &item;
    }
  }

  // Constructor for dynamically sized containers (std::vector, etc.)
  MenuDef(
      std::span<MenuItem> children,
      RefreshFn set_items = [](MenuController &, bool) {},
      MenuLabel *title = nullptr)
      : Title(title), SetItems(std::move(set_items)),
        NumItems(static_cast<uint8_t>(children.size())) {
    assert(children.size() <= WINITEM_MAX);
    for (size_t i = 0; i < children.size(); i++) {
      ItemPtr[i] = &children[i];
    }
  }

  MenuDef(RefreshFn set_items, std::initializer_list<MenuItem *> children)
      : Title(nullptr), SetItems(std::move(set_items)),
        NumItems(children.size()) {
    for (size_t i = 0; const auto &item : children) {
      ItemPtr[i++] = item;
    }
  }

  // For dynamic pointer arrays (std::vector<MenuItem *>, etc.)
  MenuDef(RefreshFn set_items, std::span<MenuItem *const> children,
          MenuLabel *title = nullptr)
      : Title(title), SetItems(std::move(set_items)),
        NumItems(static_cast<uint8_t>(children.size())) {
    assert(children.size() <= WINITEM_MAX);
    for (size_t i = 0; i < children.size(); i++) {
      ItemPtr[i] = children[i];
    }
  }

  // Returns the maximum number of items among all submenus.
  [[nodiscard]] uint8_t MaxItems() const;
};

// Window group
class MenuController {
public:
  explicit MenuController(MenuDef &parent) : Parent(parent) {}

  // --- Initialization ---
  void Init(PIXEL_COORD w);                    // Prepares text rendering.
  void Open(WINDOW_POINT topleft, int select); // Initialize command window
  void OpenCentered(PIXEL_COORD w, int select);

  // --- Per-frame processing ---
  void Tick(INPUT_BITS key); // Replaces CWinMove (takes key as argument)
  void Draw();               // Replaces CWinDraw

  // --- Search ---
  MenuDef *SearchActive(); // Replaces CWinSearchActive

  // --- State inspection ---
  bool Active() const { return State != CWIN_DEAD; }
  uint8_t Depth() const { return SelectDepth; }
  uint8_t CurrentSelection() const { return Select[SelectDepth]; }
  uint8_t SelectionAt(uint8_t depth) const { return Select[depth]; }
  INPUT_BITS LastKey() const { return OldKey; }
  int Y() const { return y; }

  // --- State manipulation (callbacks / scroll menus) ---
  void SetCurrentSelection(uint8_t sel) { Select[SelectDepth] = sel; }
  void PopLevel() {
    if (SelectDepth > 0)
      SelectDepth--;
  }
  void Close() { State = CWIN_DEAD; }
  void SetLastKey(INPUT_BITS key) { OldKey = key; }
  void SetY(int new_y) { y = new_y; }
  void AdjustYForTallMenu(int baseline_y, int max_visible);

  MenuDef &Root() { return Parent; }

private:
  void KeyEvent(INPUT_BITS key); // Replaces CWinKeyEvent

  MenuDef &Parent;  // Parent window
  int x = 0, y = 0; // Top-left coordinates of window
  PIXEL_COORD W = 0;
  uint32_t Count = 0;                // Frame counter
  uint8_t Select[WINDOW_DEPTH] = {}; // Selected item stack
  uint8_t SelectDepth = 0;           // Stack pointer for selected items
  uint8_t State = CWIN_DEAD;         // State

  INPUT_BITS OldKey = 0;      // Previously pressed key
  uint8_t KeyCount = 0;       // Keyboard wait
  uint8_t FastRepeatWait = 0; // FAST_REPEAT item wait
  bool FirstWait = false;     // Wait for first key release

  TEXTRENDER_RECT_ID TRRs[1 + WINITEM_MAX] = {}; // Initialized in Init().
};

// [Functions]

// Command window processing
bool CWinExitFn(MenuController &ctrl, INPUT_BITS key);

// Calculates the rendered width of the given text in the menu item font,
// without any padding.
PIXEL_SIZE CWinTextExtent(std::string_view str);

// Calculates the rendered width of a whole padded menu item with the given
// text.
PIXEL_SIZE CWinItemExtent(std::string_view str);
