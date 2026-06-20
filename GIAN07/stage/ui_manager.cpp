///
/// UIManager - UI manager
///

#include "ui_manager.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>

#include "config.h"
#include "demo_play.h"
#include "game/bgm.h"
#include "game_main.h"
#include "gameflow/demo_manager.h"
#include "gameflow/gameflow_manager.h"

#include <algorithm>

// Only global instance
UIManager UI;

// ---------------------------------------------------------------------------
// BGM Pack / Replay Files constants
// ---------------------------------------------------------------------------

namespace {
constexpr std::string_view BGMPackTitle = " BGM pack";
constexpr std::string_view BGMPackTitleFmt = " BGM pack ({}/{})";
constexpr std::string_view BGMPackTitleNone = "<使用しない>";
constexpr std::string_view BGMPackTitleDownload = "<Download>";
constexpr auto BGMPackHelpNone = "デフォルトのMIDIサントラに戻ります";
constexpr auto BGMPackHelpDownload = "収録のサントラをダウンロードします";

constexpr std::string_view ReplayFilesTitle = "    Replay Files";
} // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UIManager::UIManager()
    : main_window_(main_panel_.Menu()),

      exit_title_("    終了するの？"),
      exit_items_{
          MenuItem{"  Save && Exit  ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       Demos.SaveReplayAll(false);
                       GameExit();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   お っ け ～ ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       Demos.save_all_enable = false;
                       GameExit();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   だ め だ め", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameRestart();
                       return false;
                     }
                     return true;
                   }},
      },
      exit_menu_(
          std::span(exit_items_), [](MenuController &, bool) {}, &exit_title_),
      exit_window_(exit_menu_),

      continue_title_(" Ｃｏｎｔｉｎｕｅ？"),
      continue_items_{
          MenuItem{"   お っ け ～", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameContinue();
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   や だ や だ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       GameFlow.NameRegistInit(true);
                       return false;
                     }
                     return true;
                   }},
      },
      continue_menu_(
          std::span(continue_items_), [](MenuController &, bool) {},
          &continue_title_),
      continue_window_(continue_menu_),

      game_over_save_title_("  Save Replay?"),
      game_over_save_items_{
          MenuItem{"   お っ け ～ ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       Demos.SaveReplayAll(false);
                       GameExit(true);
                       return false;
                     }
                     return true;
                   }},
          MenuItem{"   や だ や だ", "",
                   [](MenuController &, INPUT_BITS key) {
                     if (Input_IsOK(key)) {
                       Demos.save_all_enable = false;
                       GameExit(true);
                       return false;
                     }
                     return true;
                   }},
      },
      game_over_save_menu_(
          std::span(game_over_save_items_), [](MenuController &, bool) {},
          &game_over_save_title_),
      game_over_save_window_(game_over_save_menu_),

      bgm_title_item_(""),
      bgm_pack_scroll_menu_(
          bgm_title_item_, [this]() { return BGMPackListSize(); },
          [this](MenuItem &ret, size_t g, size_t s) {
            BGMPackGenerate(ret, g, s);
          },
          [this](MenuController &c, INPUT_BITS k, size_t s) {
            return BGMPackHandle(c, k, s);
          },
          20),
      bgm_pack_window_(bgm_pack_scroll_menu_.Menu()),

      replay_title_item_(""),
      replay_files_scroll_menu_(
          replay_title_item_, [this]() { return ReplayFilesListSize(); },
          [this](MenuItem &ret, size_t g, size_t s) {
            ReplayFilesGenerate(ret, g, s);
          },
          [this](MenuController &c, INPUT_BITS k, size_t s) {
            return ReplayFilesHandle(c, k, s);
          },
          20),
      replay_files_window_(replay_files_scroll_menu_.Menu()) {}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void UIManager::InitMain() {
  main_panel_.Init();
  main_window_.Init(140);
}

void UIManager::InitExit() {
  exit_window_.Init(140);
  game_over_save_window_.Init(140);
}

void UIManager::InitContinue() { continue_window_.Init(140); }

// ---------------------------------------------------------------------------
// Title screen active menu
// ---------------------------------------------------------------------------

MenuController *UIManager::ActiveMenu() {
  if (replay_files_window_.Active()) {
    return &replay_files_window_;
  }
  if (bgm_pack_window_.Active()) {
    return &bgm_pack_window_;
  }
  return &main_window_;
}

void UIManager::MsgHelp() {
  if (auto *active = ActiveMenu()) {
    msg_window_.Help(active);
  }
}

// ---------------------------------------------------------------------------
// BGM Pack scroll menu
// ---------------------------------------------------------------------------

size_t UIManager::BGMPackListSize() { return (1 + bgm_packs_.size() + 1); }

void UIManager::BGMPackGenerate(MenuItem &ret, size_t generated,
                                size_t selected) {
  const auto sel_none = 0;
  const auto sel_download = (BGMPackListSize() - 1);

  if (generated == sel_none) {
    ret.Title = BGMPackTitleNone.data();
    ret.Help = BGMPackHelpNone;
  } else if (generated == sel_download) {
    ret.Title = BGMPackTitleDownload.data();
    ret.Help = BGMPackHelpDownload;
  } else {
    ret.Title = bgm_packs_[generated - 1].c_str();
    ret.Help = "";
  }

  ret.Flags = ((generated == bgm_sel_at_open_) ? MenuFlags::HIGHLIGHT
                                               : MenuFlags::NONE);

  if (generated == selected) {
    if ((generated == sel_none) || (generated == sel_download)) {
      bgm_title_text_.Set(BGMPackTitle);
    } else {
      bgm_title_text_.Format(BGMPackTitleFmt, generated, bgm_packs_.size());
    }
    bgm_title_item_.Title = bgm_title_text_.Lit();
  }
}

bool UIManager::BGMPackHandle(MenuController &ctrl, INPUT_BITS key,
                              size_t selected) {
  if (Input_IsOK(key)) {
    const auto sel_download = (BGMPackListSize() - 1);
    if (selected == sel_download) {
      SDL_OpenURL(BGMPackSoundtrackURL);
    } else {
      if (selected == 0) {
        ConfigDat.bgm_pack.clear();
      } else {
        ConfigDat.bgm_pack = bgm_packs_[selected - 1];
      }
      main_panel_.Sound().Refresh(ctrl, false);
      BGM_PackSet(ConfigDat.bgm_pack);
    }
    return false;
  }
  return true;
}

void UIManager::OpenBGMPack() {
  bgm_title_item_.Title = bgm_title_text_.Lit();
  PIXEL_COORD w = CWinItemExtent(BGMPackTitleFmt).w;
  w = (std::max)(w, CWinTextExtent(BGMPackTitleDownload).w);
  w = (std::max)(w, CWinTextExtent(BGMPackTitleNone).w);
  bgm_packs_.clear();
  bgm_packs_.reserve(BGM_PackCount());
  BGM_PackForeach([this](const auto pack) { bgm_packs_.emplace_back(pack); });
  std::ranges::sort(bgm_packs_);
  bgm_sel_at_open_ = 0;
  for (size_t i = 1; const auto &pack : bgm_packs_) {
    if (pack == ConfigDat.bgm_pack) {
      bgm_sel_at_open_ = i;
    }
    w = (std::max)(w,
                   CWinItemExtent(std::string_view{
                                      std::bit_cast<const char *>(pack.c_str()),
                                      pack.size()})
                       .w);
    i++;
  }
  w = (std::min)(w, GRP_RES.w);

  bgm_pack_scroll_menu_.Init(bgm_pack_window_, bgm_sel_at_open_, &main_window_);
  bgm_pack_window_.Init(w);
  bgm_pack_window_.OpenCentered(w, bgm_pack_window_.SelectionAt(0));
}

// ---------------------------------------------------------------------------
// Replay Files scroll menu
// ---------------------------------------------------------------------------

size_t UIManager::ReplayFilesListSize() {
  return (!replay_files_.empty() ? replay_files_.size() + 1 : 2);
}

void UIManager::ReplayFilesGenerate(MenuItem &ret, size_t generated,
                                    size_t selected) {
  if (generated == (ReplayFilesListSize() - 1)) {
    ret.Title = " Exit";
    ret.Help = "一つ前のメニューにもどります";
  } else if (generated < replay_files_.size()) {
    ret.Title =
        reinterpret_cast<const char *>(replay_files_[generated].c_str());
    ret.Help = "Play replay file";
  } else {
    ret.Title = " No replays found";
    ret.Help = "";
  }
  ret.Flags = MenuFlags::NONE;
  if (generated == selected) {
    if (selected < replay_files_.size()) {
      replay_title_text_.Set(
          reinterpret_cast<const char *>(replay_files_[selected].c_str()));
    } else {
      replay_title_text_.Set(ReplayFilesTitle);
    }
    replay_title_item_.Title = replay_title_text_.Lit();
  }
}

bool UIManager::ReplayFilesHandle(MenuController &, INPUT_BITS key,
                                  size_t selected) {
  if (Input_IsOK(key)) {
    if (selected == (ReplayFilesListSize() - 1)) {
      return false;
    }
    if (selected < replay_files_.size()) {
      ::Demos.pending_replay_file = replay_files_[selected];
      return false;
    }
  }
  return true;
}

void UIManager::OpenReplayFiles() {
  replay_title_item_.Title = replay_title_text_.Lit();

  // Scan replay files
  replay_files_.clear();
  SDL_EnumerateDirectory(
      ".",
      [](void *ctx, const char *, const char *name) {
        if (strstr(name, "replay_") == name && strstr(name, ".DAT")) {
          auto &files = *static_cast<std::vector<std::string> *>(ctx);
          files.emplace_back(name);
        }
        return SDL_ENUM_CONTINUE;
      },
      &replay_files_);
  std::ranges::sort(replay_files_, std::greater{});

  PIXEL_COORD w = CWinItemExtent(ReplayFilesTitle).w;
  for (const auto &f : replay_files_) {
    w = (std::max)(w, CWinItemExtent(
                          std::string_view{
                              std::bit_cast<const char *>(f.c_str()), f.size()})
                          .w);
  }
  w = (std::max)(w, CWinItemExtent(" Exit").w);
  w = (std::min)(w, GRP_RES.w);

  replay_files_scroll_menu_.Init(replay_files_window_, 0, &main_window_);
  replay_files_window_.Init(w);
  replay_files_window_.OpenCentered(w, replay_files_window_.SelectionAt(0));
}
