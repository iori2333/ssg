/*                                                                           */
/*   WINDOWCTRL.cpp   ウィンドウの定義＆管理                                 */
/*                                                                           */
/*   パネルクラス化 + std::format 化により、旧来の namespace +                */
/*   静的 char[] + sprintf パターンを廃止した。                                */
/*                                                                           */

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>

#include "config.h"
#include "demo_play.h"
#include "entry.h"
#include "game/bgm.h"
#include "game/snd.h"
#include "game_main.h"
#include "gameflow/demo_manager.h"
#include "gameflow/gameflow_manager.h"
#include "level.h"
#include "loader.h"
#include "menu/panels.h"
#include "menu/scroll_menu.h"
#include "music.h"
#include "platform/input.h"
#include "ui_manager.h"
#include "window_ctrl.h"
#include "window_sys.h"

#include <algorithm>

// ---------------------------------------------------------------------------
// 前方宣言
// ---------------------------------------------------------------------------

static MainMenuPanel MainPanel;
extern ScrollMenu BGMPackScrollMenu;
extern ScrollMenu ReplayFilesScrollMenu;
extern MenuController MainWindow;
extern MenuController BGMPackWindow;
extern MenuController ReplayFilesWindow;

// ---------------------------------------------------------------------------
// BGMPack スクロールメニュー
// ---------------------------------------------------------------------------

namespace BGMPack {
const char *SOUNDTRACK_URL =
    "https://github.com/nmlgc/BGMPacks/releases/tag/2024-10-05";
constexpr Narrow::string_view TITLE = " BGM pack";
constexpr Narrow::string_view TITLE_FMT = " BGM pack ({}/{})";
constexpr Narrow::string_view TITLE_NONE = "<使用しない>";
constexpr Narrow::string_view TITLE_DOWNLOAD = "<Download>";
constexpr auto HELP_NONE = "デフォルトのMIDIサントラに戻ります";
constexpr auto HELP_DOWNLOAD = "収録のサントラをダウンロードします";

MenuText TitleText;
MenuLabel TitleItem{""};

std::vector<std::u8string> Packs;
size_t SelAtOpen = 0;

size_t ListSize() { return (1 + Packs.size() + 1); }

size_t SelNone() { return 0; }
size_t SelDownload() { return (ListSize() - 1); }

void Generate(MenuItem &ret, size_t generated, size_t selected) {
  if (generated == SelNone()) {
    ret.Title = TITLE_NONE.data();
    ret.Help = HELP_NONE;
  } else if (generated == SelDownload()) {
    ret.Title = TITLE_DOWNLOAD.data();
    ret.Help = HELP_DOWNLOAD;
  } else {
    ret.Title = Narrow::literal{
        reinterpret_cast<const char *>(Packs[generated - 1].c_str())};
    ret.Help = "";
  }

  ret.Flags =
      ((generated == SelAtOpen) ? MenuFlags::HIGHLIGHT : MenuFlags::NONE);

  if (generated == selected) {
    if ((generated == SelNone()) || (generated == SelDownload())) {
      TitleText.Set(TITLE);
    } else {
      TitleText.Format(TITLE_FMT, generated, Packs.size());
    }
    TitleItem.Title = TitleText.Lit();
  }
}

bool Handle(MenuController &ctrl, INPUT_BITS key, size_t selected) {
  if (Input_IsOK(key)) {
    if (selected == SelDownload()) {
      SDL_OpenURL(SOUNDTRACK_URL);
    } else {
      if (selected == SelNone()) {
        ConfigDat.BGMPack.clear();
      } else {
        ConfigDat.BGMPack = Packs[selected - 1];
      }
      MainPanel.Sound().Refresh(ctrl, false);
      BGM_PackSet(ConfigDat.BGMPack);
    }
    return false;
  }
  return true;
}

void Open() {
  TitleItem.Title = TitleText.Lit();
  PIXEL_COORD w = CWinItemExtent(TITLE_FMT).w;
  w = (std::max)(w, CWinTextExtent(TITLE_DOWNLOAD).w);
  w = (std::max)(w, CWinTextExtent(TITLE_NONE).w);
  Packs.clear();
  Packs.reserve(BGM_PackCount());
  BGM_PackForeach([](const auto pack) { Packs.emplace_back(pack); });
  std::ranges::sort(Packs);
  SelAtOpen = SelNone();
  for (size_t i = 1; const auto &pack : Packs) {
    if (pack == ConfigDat.BGMPack) {
      SelAtOpen = i;
    }
    w = (std::max)(w, CWinItemExtent(pack).w);
    i++;
  }
  w = (std::min)(w, GRP_RES.w);

  BGMPackScrollMenu.Init(BGMPackWindow, SelAtOpen, &MainWindow);
  BGMPackWindow.Init(w);
  BGMPackWindow.OpenCentered(w, BGMPackWindow.SelectionAt(0));
}
} // namespace BGMPack

// ---------------------------------------------------------------------------
// ReplayFiles スクロールメニュー
// ---------------------------------------------------------------------------

namespace ReplayFiles {
constexpr Narrow::string_view TITLE = "    Replay Files";

MenuText TitleText;
MenuLabel TitleItem{""};

std::vector<std::u8string> Files;

size_t ListSize() { return (!Files.empty() ? Files.size() + 1 : 2); }

void ScanFiles() {
  Files.clear();
  SDL_EnumerateDirectory(
      ".",
      [](void *ctx, const char *, const char *name) {
        if (strstr(name, "replay_") == name && strstr(name, ".DAT")) {
          auto &files = *static_cast<decltype(Files) *>(ctx);
          files.emplace_back(reinterpret_cast<const char8_t *>(name));
        }
        return SDL_ENUM_CONTINUE;
      },
      &Files);
  std::ranges::sort(Files, std::greater{});
}

void Generate(MenuItem &ret, size_t generated, size_t selected) {
  if (generated == (ListSize() - 1)) {
    ret.Title = " Exit";
    ret.Help = "一つ前のメニューにもどります";
  } else if (generated < Files.size()) {
    ret.Title = Narrow::literal{
        reinterpret_cast<const char *>(Files[generated].c_str())};
    ret.Help = "Play replay file";
  } else {
    ret.Title = " No replays found";
    ret.Help = "";
  }
  ret.Flags = MenuFlags::NONE;
  if (generated == selected) {
    if (selected < Files.size()) {
      TitleText.Set(reinterpret_cast<const char *>(Files[selected].c_str()));
    } else {
      TitleText.Set(TITLE);
    }
    TitleItem.Title = TitleText.Lit();
  }
}

bool Handle(MenuController &, INPUT_BITS key, size_t selected) {
  if (Input_IsOK(key)) {
    if (selected == (ListSize() - 1)) {
      return false;
    }
    if (selected < Files.size()) {
      ::Demos.pending_replay_file = Files[selected];
      return false;
    }
  }
  return true;
}

void Open() {
  TitleItem.Title = TitleText.Lit();
  ScanFiles();

  PIXEL_COORD w = CWinItemExtent(TITLE).w;
  for (const auto &f : Files) {
    w = (std::max)(w, CWinItemExtent(f).w);
  }
  w = (std::max)(w, CWinItemExtent(" Exit").w);
  w = (std::min)(w, GRP_RES.w);

  ReplayFilesScrollMenu.Init(ReplayFilesWindow, 0, &MainWindow);
  ReplayFilesWindow.Init(w);
  ReplayFilesWindow.OpenCentered(w, ReplayFilesWindow.SelectionAt(0));
}
} // namespace ReplayFiles

// ---------------------------------------------------------------------------
// ダイアログメニュー (Exit / Continue / GameOverSave)
// ---------------------------------------------------------------------------

namespace {
MenuLabel ExitTitle{"    終了するの？"};
MenuItem ExitItems[] = {
    MenuItem{"  Save && Exit  ", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         Demos.SaveReplayAll(false);
         GameExit();
         return false;
       }
       return true;
     }},
    MenuItem{"   お っ け ～ ", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         Demos.save_all_enable = false;
         GameExit();
         return false;
       }
       return true;
     }},
    MenuItem{"   だ め だ め", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         GameRestart();
         return false;
       }
       return true;
     }},
};
MenuDef ExitMenu(std::span(ExitItems), [](MenuController &, bool) {}, &ExitTitle);

MenuLabel ContinueTitle{" Ｃｏｎｔｉｎｕｅ？"};
MenuItem ContinueItems[] = {
    MenuItem{"   お っ け ～", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         GameContinue();
         return false;
       }
       return true;
     }},
    MenuItem{"   や だ や だ", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         GameFlow.NameRegistInit(true);
         return false;
       }
       return true;
     }},
};
MenuDef ContinueMenu(std::span(ContinueItems), [](MenuController &, bool) {},
                     &ContinueTitle);

MenuLabel GameOverSaveTitle{"  Save Replay?"};
MenuItem GameOverSaveItems[] = {
    MenuItem{"   お っ け ～ ", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         Demos.SaveReplayAll(false);
         GameExit(true);
         return false;
       }
       return true;
     }},
    MenuItem{"   や だ や だ", "", [](MenuController &, INPUT_BITS key) {
       if (Input_IsOK(key)) {
         Demos.save_all_enable = false;
         GameExit(true);
         return false;
       }
       return true;
     }},
};
MenuDef GameOverSaveMenu(std::span(GameOverSaveItems), [](MenuController &, bool) {},
                         &GameOverSaveTitle);
} // namespace

// (MainPanel は前方宣言で定義済み)

// ---------------------------------------------------------------------------
// グローバル変数(公開)
// ---------------------------------------------------------------------------

MenuController MainWindow(MainPanel.Menu());
MenuController ExitWindow(ExitMenu);
MenuController ContinueWindow(ContinueMenu);
ScrollMenu BGMPackScrollMenu(BGMPack::TitleItem, &BGMPack::ListSize,
                             &BGMPack::Generate, &BGMPack::Handle, 20);
MenuController BGMPackWindow(BGMPackScrollMenu.Menu());
MenuController GameOverSaveWindow(GameOverSaveMenu);
ScrollMenu ReplayFilesScrollMenu(ReplayFiles::TitleItem, &ReplayFiles::ListSize,
                                 &ReplayFiles::Generate, &ReplayFiles::Handle,
                                 20);
MenuController ReplayFilesWindow(ReplayFilesScrollMenu.Menu());

// UIManager にメニューコントローラを登録
namespace {
struct UIBindInit {
  UIBindInit() {
    UI.Bind(MainWindow, ExitWindow, ContinueWindow, BGMPackWindow,
            GameOverSaveWindow, ReplayFilesWindow);
  }
} ui_bind_init;
} // namespace

// ---------------------------------------------------------------------------
// 初期化関数
// ---------------------------------------------------------------------------

void InitMainWindow() {
#ifdef SUPPORT_GRP_API
  const auto grp_api_count = GrpBackend_APICount();
  if (grp_api_count >= 2) {
    assert(grp_api_count <= 8);
    auto &api = MainPanel.Config().Graphics().Api();
    auto &menu = api.Menu();
    auto *menu_p = menu.ItemPtr;

    *(menu_p++) = &api.ApiItem();
    for (const auto i : std::views::iota(0, grp_api_count)) {
      const auto driver_str = GrpBackend_APIString(i);
      const auto label = GrpBackend_APILabel(driver_str);
      assert(!label.empty());
      api.Items()[i] = MenuItem(
          reinterpret_cast<const char *>(label.data()),
          "Select to override default API selection",
          ApiPanel::FnOverride);
      *(menu_p++) = &api.Items()[i];
    }
    *(menu_p++) = &SubmenuExitItem;
    menu.NumItems = static_cast<uint8_t>(std::distance(menu.ItemPtr, menu_p));
  } else {
    MainPanel.Config().Graphics().ApiMenuItem().Flags |= MenuFlags::DISABLED;
  }
#endif

  MainWindow.Init(140);
}

void InitExitWindow() {
  ExitWindow.Init(140);
  GameOverSaveWindow.Init(140);
}

void InitContinueWindow() { ContinueWindow.Init(140); }
