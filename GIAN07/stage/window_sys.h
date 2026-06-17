/*                                                                           */
/*   WINDOWSYS.h   コマンドウィンドウ処理                                    */
/*                                                                           */
/*                                                                           */

#pragma once

#include <functional>
#include <utility>

#include "game/enum_flags.h"
#include "game/input.h"
#include "game/text.h"

///// [更新履歴] /////
/*
 * 2000/07/26 : 操作性を改善した
 *
 * 2000/02/28 : メッセージウィンドウの半透明部を高速化
 * 2000/02/10 : PWINDOW->WINDOWSYS
 *            : WINDOWSYS
 * において、コマンドウィンドウは、スタックで選択状態を保持する。 :
 * ウィンドウを動かしたり枠を付けたりするのはあとでね...
 *
 * 2000/01/31 : 開発はじめ
 *
 */

// Constants
// ---------

// 最大数に関する定数 //
inline constexpr auto WINITEM_MAX = 20;  // 項目の最大数
inline constexpr auto WINDOW_DEPTH = 10; // ウィンドウの深さ
inline constexpr auto MSG_HEIGHT = 5;    // メッセージウィンドウの高さ

// コマンドウィンドウの状態 //
inline constexpr auto CWIN_DEAD = 0x00;   // 使用されていない
inline constexpr auto CWIN_FREE = 0x01;   // 入力待ち状態
inline constexpr auto CWIN_OPEN = 0x02;   // 項目移動中(進む)
inline constexpr auto CWIN_CLOSE = 0x03;  // 項目移動中(戻る)
inline constexpr auto CWIN_NEXT = 0x04;   // 次のウィンドウに移行中
inline constexpr auto CWIN_BEFORE = 0x05; // 前のウィンドウに移行中
inline constexpr auto CWIN_INIT = 0xff;   // 初期化処理中

constexpr auto CWIN_FONT = FONT_ID::SMALL;

constexpr PIXEL_COORD CWIN_ITEM_LEFT = 8;
constexpr PIXEL_COORD CWIN_ITEM_H = 16;
constexpr PIXEL_COORD CWIN_MAX_H = ((WINITEM_MAX + 1) * CWIN_ITEM_H);

constexpr PIXEL_COORD FACE_W = 96;
constexpr PIXEL_COORD FACE_H = 96;

// メッセージウィンドウ・コマンド //
inline constexpr auto MWCMD_SMALLFONT = 0x00;  // スモールフォントを使用する
inline constexpr auto MWCMD_NORMALFONT = 0x01; // ノーマルフォントを使用する
inline constexpr auto MWCMD_LARGEFONT = 0x02;  // ラージフォントを使用する
inline constexpr auto MWCMD_NEWPAGE = 0x03;    // 改ページする

// メッセージウィンドウの状態 //
inline constexpr auto MWIN_DEAD = 0x00;  // 使用されていない
inline constexpr auto MWIN_OPEN = 0x01;  // オープン中
inline constexpr auto MWIN_CLOSE = 0x02; // クローズ中
inline constexpr auto MWIN_FREE = 0x03;  // 待ち状態

// メッセージウィンドウ(顔の状態) //
inline constexpr auto MFACE_NONE = 0x00;  // 表示されていない
inline constexpr auto MFACE_OPEN = 0x01;  // オープン中
inline constexpr auto MFACE_CLOSE = 0x02; // クローズ中
inline constexpr auto MFACE_NEXT = 0x03;  // 次の顔へ
inline constexpr auto MFACE_WAIT = 0x04;  // 待ち状態

// その他の定数 //
inline constexpr auto CWIN_KEYWAIT = 8;
// ---------

///// [構造体] /////

enum class WINDOW_FLAGS : uint8_t {
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

struct WINDOW_MENU;
class MenuController;

// Shared data for menu titles and choices.
struct WINDOW_LABEL {
  Narrow::literal Title; // タイトル文字列へのポインタ(実体ではない！)

  WINDOW_FLAGS Flags = WINDOW_FLAGS::NONE;

  // Required for forcing the item to be re-rendered after a flag change.
  WINDOW_FLAGS FlagsPrev = WINDOW_FLAGS::FORCE_RERENDER;

  constexpr WINDOW_LABEL(const Narrow::literal title = "",
                         WINDOW_FLAGS flags = WINDOW_FLAGS::NONE) noexcept
      : Title(title), Flags(flags) {}
};

// 子ウィンドウの情報 //
struct WINDOW_CHOICE : public WINDOW_LABEL {
  // コールバック関数の型 //
  using ActionFn = std::function<bool(MenuController &, INPUT_BITS)>;
  using AdjustFn = std::function<void(MenuController &, int_fast8_t)>;

  // Raw pointer aliases for constructor overloading disambiguation.
  using ActionFnPtr = bool (*)(MenuController &, INPUT_BITS);
  using AdjustFnPtr = void (*)(MenuController &, int_fast8_t);

  Narrow::literal Help; // ヘルプ文字列へのポインタ(これも実体ではない)

  // 特殊処理用コールバック関数(未使用なら空)
  ActionFn CallBackFn;

  // Callback function for options (falls back on [CallBackFn] if empty)
  AdjustFn OptionFn;

  WINDOW_MENU *Submenu = nullptr;

  WINDOW_CHOICE(const Narrow::literal title = "",
                const Narrow::literal help = "",
                ActionFnPtr callback_fn = nullptr,
                WINDOW_FLAGS flags = WINDOW_FLAGS::NONE)
      : WINDOW_LABEL(title, flags), Help(help), CallBackFn(callback_fn) {
    if (!CallBackFn) {
      Flags |= WINDOW_FLAGS::DISABLED;
    }
  }

  WINDOW_CHOICE(const Narrow::literal title,
                const Narrow::literal help,
                AdjustFnPtr option_fn,
                WINDOW_FLAGS flags = WINDOW_FLAGS::NONE)
      : WINDOW_LABEL(title, flags), Help(help), OptionFn(option_fn) {}

  WINDOW_CHOICE(const Narrow::literal title,
                const Narrow::literal help,
                WINDOW_FLAGS flags)
      : WINDOW_LABEL(title, flags), Help(help) {}

  WINDOW_CHOICE(const Narrow::literal title,
                const Narrow::literal help,
                WINDOW_MENU &submenu)
      : WINDOW_LABEL(title), Help(help), Submenu(&submenu) {}

  void SetActive(bool active);
};

// Dynamic collection of menu items at a single hierarchy level.
struct WINDOW_MENU {
  using RefreshFn = std::function<void(MenuController &, bool tick)>;

  WINDOW_LABEL *Title = nullptr;
  WINDOW_CHOICE *ItemPtr[WINITEM_MAX] = {nullptr}; // 次の項目へのポインタ
  RefreshFn SetItems = [](MenuController &, bool) {};
  uint8_t NumItems = 0; // 項目数(<ITEM_MAX)

  WINDOW_MENU() = default;

  template <size_t N>
  WINDOW_MENU(
      std::span<WINDOW_CHOICE, N> children,
      RefreshFn set_items = [](MenuController &, bool) {},
      WINDOW_LABEL *title = nullptr) noexcept
      : Title(title), SetItems(std::move(set_items)), NumItems(N) {
    static_assert(N <= WINITEM_MAX);
    for (size_t i = 0; auto &item : children) {
      ItemPtr[i++] = &item;
    }
  }

  WINDOW_MENU(RefreshFn set_items,
              std::initializer_list<WINDOW_CHOICE *> children)
      : Title(nullptr), SetItems(std::move(set_items)), NumItems(children.size()) {
    for (size_t i = 0; const auto &item : children) {
      ItemPtr[i++] = item;
    }
  }

  // Returns the maximum number of items among all submenus.
  [[nodiscard]] uint8_t MaxItems() const;
};

// ウィンドウ群 //
class MenuController {
public:
  explicit MenuController(WINDOW_MENU &parent) : Parent(parent) {}

  // --- 初期化 ---
  void Init(PIXEL_COORD w);                    // Prepares text rendering.
  void Open(WINDOW_POINT topleft, int select); // コマンドウィンドウの初期化
  void OpenCentered(PIXEL_COORD w, int select);

  // --- 毎フレーム処理 ---
  void Tick(INPUT_BITS key); // CWinMove を置き換え (キーを引数で受け取る)
  void Draw();               // CWinDraw を置き換え

  // --- 検索 ---
  WINDOW_MENU *SearchActive(); // CWinSearchActive を置き換え

  // --- 状態検査 ---
  bool Active() const { return State != CWIN_DEAD; }
  uint8_t Depth() const { return SelectDepth; }
  uint8_t CurrentSelection() const { return Select[SelectDepth]; }
  uint8_t SelectionAt(uint8_t depth) const { return Select[depth]; }
  INPUT_BITS LastKey() const { return OldKey; }
  int Y() const { return y; }

  // --- 状態操作 (コールバック / スクロールメニュー用) ---
  void SetCurrentSelection(uint8_t sel) { Select[SelectDepth] = sel; }
  void PopLevel() {
    if (SelectDepth > 0) SelectDepth--;
  }
  void Close() { State = CWIN_DEAD; }
  void SetLastKey(INPUT_BITS key) { OldKey = key; }
  void SetY(int new_y) { y = new_y; }
  void AdjustYForTallMenu(int baseline_y, int max_visible);

  WINDOW_MENU &Root() { return Parent; }

private:
  void KeyEvent(INPUT_BITS key); // CWinKeyEvent を置き換え

  WINDOW_MENU &Parent;                       // 親ウィンドウ
  int x = 0, y = 0;                          // ウィンドウ左上の座標
  PIXEL_COORD W = 0;
  uint32_t Count = 0;                        // フレームカウンタ
  uint8_t Select[WINDOW_DEPTH] = {};         // 選択中の項目スタック
  uint8_t SelectDepth = 0;                   // 選択中の項目に対するＳＰ
  uint8_t State = CWIN_DEAD;                 // 状態

  INPUT_BITS OldKey = 0;                     // 前に押されていたキー
  uint8_t KeyCount = 0;                      // キーボードウェイト
  uint8_t FastRepeatWait = 0;                // FAST_REPEAT 項目のウェイト
  bool FirstWait = false;                    // 最初のキー解放待ち

  TEXTRENDER_RECT_ID TRRs[1 + WINITEM_MAX] = {}; // Init() で初期化。
};

// 後方互換エイリアス
using WINDOW_SYSTEM = MenuController;

///// [ 関数 ] /////

// コマンドウィンドウ処理 //
void CWinMove(WINDOW_SYSTEM *ws); // コマンドウィンドウを１フレーム動作させる
void CWinDraw(WINDOW_SYSTEM *ws); // コマンドウィンドウの描画
bool CWinExitFn(MenuController &ctrl, INPUT_BITS key);

// アクティブなウィンドウを探す
WINDOW_MENU *CWinSearchActive(WINDOW_SYSTEM *ws);

// Calculates the rendered width of the given text in the menu item font,
// without any padding.
PIXEL_SIZE CWinTextExtent(Narrow::string_view str);

// Calculates the rendered width of a whole padded menu item with the given
// text.
PIXEL_SIZE CWinItemExtent(Narrow::string_view str);

// メッセージウィンドウ処理 //

enum class MSG_WINDOW_FLAGS : uint8_t {
  NONE = 0x0,
  WITH_FACE = 0x1, // Pads all text to leave room for a face portrait.
  CENTER = 0x2,    // Horizontally centers all text.
  HAS_BITFLAG_OPERATORS = 3,
};

// Prepares text rendering for a window with the given dimensions.
void MWinInit(const WINDOW_LTRB &rc,
              MSG_WINDOW_FLAGS flags = MSG_WINDOW_FLAGS::NONE);

void MWinOpen();       // メッセージウィンドウをオープンする
void MWinClose();      // メッセージウィンドウをクローズする
void MWinForceClose(); // メッセージウィンドウを強制クローズする
void MWinMove();       // メッセージウィンドウを動作させる(後で上と統合する)
void MWinDraw();       // メッセージウィンドウを描画する(上に同じ)

void MWinMsg(Narrow::string_view str); // メッセージ文字列を送る
void MWinFace(uint8_t faceID);         // 顔をセットする
void MWinCmd(uint8_t cmd);             // コマンドを送る

void MWinHelp(WINDOW_SYSTEM *ws); // メッセージウィンドウにヘルプ文字列を送る
