///
/// MenuBuilder — Tree construction for the main menu and all settings
///

#include <algorithm>
#include <chrono>
#include <compare>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>

#include "menu_builder.h"
#include "menu_controller.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "audio/snd.h"
#include "audio/volume.h"
#include "core/config.h"
#include "core/graphics_settings.h"
#include "core/level.h"
#include "data/sfx_manager.h"
#include "gameflow/gameflow_manager.h"
#include "gfx/graphics_backend.h"
#include "music_room/music_room.h"
#include "sys/input.h"
#include "track_manager/track_manager.h"
#include "util/enum_flags.h"

namespace menu {

namespace {

static std::string PadButtonLabel(INPUT_PAD_BUTTON v) {
  if (v > 0) {
    return std::format("Button{}", static_cast<int>(v));
  }
  return "--------";
}

static std::string FmtLabel(const char *s) { return s; }

// ---------------------------------------------------------------------------
// Difficulty
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildDifficultyMenu(GameConfig &game_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(4);

  ch.push_back(std::make_unique<ChoiceNode>(
      "PlayerStock", "残り人数を設定します", &game_cfg.player_stock, 0,
      STOCK_PLAYER_MAX,
      std::vector<std::string>{"1", "2", "3", "4", "5", "6", "7"}));

  ch.push_back(std::make_unique<ChoiceNode>(
      "BombStock", "ボムの数を設定します", &game_cfg.bomb_stock, 0,
      STOCK_BOMB_MAX,
      std::vector<std::string>{"0", "1", "2", "3", "4", "5", "6"}));

  ch.push_back(std::make_unique<ChoiceNode>(
      "Difficulty", "難易度を設定します",
      reinterpret_cast<uint8_t *>(&game_cfg.game_level),
      static_cast<uint8_t>(GameLevel::EASY),
      static_cast<uint8_t>(GameLevel::LUNATIC),
      std::vector<std::string>{FmtLabel("Easy"), FmtLabel("Normal"),
                               FmtLabel("Hard"), FmtLabel("Lunatic")}));

  ch.push_back(std::make_unique<ChoiceNode>(
      "PracticeMode", "練習模式を設定します",
      reinterpret_cast<uint8_t *>(&game_cfg.practice_mode), 0, 2,
      std::vector<std::string>{FmtLabel("Off"), FmtLabel("AutoB"),
                               FmtLabel("Invin")}));

  return std::make_unique<EntryNode>("Difficulty", "難易度に関する設定",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// Screenshot
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildScreenshotMenu(GraphicsConfig &gfx_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(2);

  std::vector<std::string> labels;
  labels.reserve(GRP_SCREENSHOT_EFFORT_MAX + 1);
  labels.push_back("BMP");
  for (auto i : std::views::iota(1U, GRP_SCREENSHOT_EFFORT_MAX + 1U)) {
    labels.push_back(std::format("WebP z{}", i - 1));
  }
  ch.push_back(std::make_unique<ChoiceNode>(
      "Format", "Screenshot format", &gfx_cfg.screenshot_effort, 0,
      GRP_SCREENSHOT_EFFORT_MAX, std::move(labels)));

  ch.push_back(std::make_unique<SeparatorNode>());

  return std::make_unique<EntryNode>(
      "Screenshots", "Customize the screenshot format", std::move(ch));
}

// ---------------------------------------------------------------------------
// Graphics API (dynamic)
// ---------------------------------------------------------------------------

static std::unique_ptr<ListNode> BuildApiMenu(GraphicsConfig &gfx_cfg) {
  auto init_sel = static_cast<int>(gfx_cfg.GraphicsParams().api);
  auto node = std::make_unique<ListNode>(
      "API", "Select rendering API", [] { return GrpBackend_APICount(); },
      [](size_t i) {
        return std::string(GrpBackend_APILabel(GrpBackend_APIString(i)));
      },
      [&gfx_cfg](size_t i) {
        XGrpTry(gfx_cfg,
                [i](auto &params) { params.api = static_cast<int>(i); });
        return false;
      },
      init_sel);
  return node;
}

// ---------------------------------------------------------------------------
// Graphics
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildGraphicsMenu(GraphicsConfig &gfx_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(10);

  auto dev = std::make_unique<ActionNode>(
      "Device", "", [](MenuController &) { return true; });
  auto *dev_raw = dev.get();
  dev->SetPoll([dev_raw, &gfx_cfg]() {
    dev_raw->SetValue(std::string(GrpBackend_DeviceLabel(gfx_cfg.device_id)));
  });
  ch.push_back(std::move(dev));

#ifdef SUPPORT_GRP_WINDOWED
  {
    static uint8_t disp_state = 0;
    auto disp = std::make_unique<ChoiceNode>(
        "Display", "Switch between window and fullscreen modes", &disp_state, 0,
        1, std::vector<std::string>{"Window", "Fullscreen"},
        [&gfx_cfg] { XGrpTryCycleDisp(gfx_cfg); });
    disp->SetPoll([&gfx_cfg]() {
      disp_state =
          gfx_cfg.GraphicsParams().FullscreenFlags().fullscreen ? 1 : 0;
    });
    ch.push_back(std::move(disp));
  }
  {
    static uint8_t fs_state = 0;
    auto fs_mode = std::make_unique<ChoiceNode>(
        "FullScr", "Fullscreen mode", &fs_state, 0, 1,
        std::vector<std::string>{"Borderless", "Exclusive"}, [&gfx_cfg] {
          XGrpTry(gfx_cfg, [](auto &params) {
            params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE;
          });
        });
    auto *fs_raw = fs_mode.get();
    fs_mode->SetPoll([fs_raw, &gfx_cfg]() {
      fs_state = !!(gfx_cfg.graphics_param_flags &
                    GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE);
      fs_raw->SetEnabled(gfx_cfg.GraphicsParams().FullscreenFlags().fullscreen);
    });
    ch.push_back(std::move(fs_mode));
  }
#endif

#ifdef SUPPORT_GRP_SCALING
  {
    auto scale = std::make_unique<ActionNode>(
        "ScaleFact", "Window scaling factor",
        [&gfx_cfg](MenuController &) {
          XGrpTryCycleScale(gfx_cfg, 0, true);
          return true;
        },
        [&gfx_cfg](MenuController &, int delta) {
          XGrpTryCycleScale(gfx_cfg, delta, true);
        });
    auto *scale_raw = scale.get();
    scale->SetPoll([scale_raw, &gfx_cfg]() {
      const auto params = gfx_cfg.GraphicsParams();
      const auto fs = params.FullscreenFlags();
      if (fs.fullscreen && !fs.exclusive) {
        switch (fs.fit) {
        case GRAPHICS_FULLSCREEN_FIT::INTEGER:
          scale_raw->SetValue("Integer");
          break;
        case GRAPHICS_FULLSCREEN_FIT::ASPECT:
          scale_raw->SetValue("Aspect");
          break;
        case GRAPHICS_FULLSCREEN_FIT::STRETCH:
          scale_raw->SetValue("Stretch");
          break;
        default:
          break;
        }
      } else {
        const auto sv = params.window_scale_4x;
        if (sv == 0) {
          scale_raw->SetValue("Screen");
        } else {
          scale_raw->SetValue(
              std::format("{:3}.{:02}x", sv / 4, (sv % 4) * 25));
        }
      }
    });
    ch.push_back(std::move(scale));

    static uint8_t sc_state = 0;
    auto sc_mode = std::make_unique<ChoiceNode>(
        "ScaleMode", "Scaling method", &sc_state, 0, 1,
        std::vector<std::string>{"FrameBuf", "Geometry"},
        [&gfx_cfg] { XGrpTryCycleScMode(gfx_cfg); });
    auto *sc_raw = sc_mode.get();
    sc_mode->SetPoll([sc_raw, &gfx_cfg]() {
      sc_state = !!(gfx_cfg.graphics_param_flags &
                    GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY);
      const auto params = gfx_cfg.GraphicsParams();
      bool exclusive = params.FullscreenFlags().fullscreen &&
                       params.FullscreenFlags().exclusive;
      sc_raw->SetEnabled(!exclusive);
    });
    ch.push_back(std::move(sc_mode));
  }
#endif

  {
    ch.push_back(std::make_unique<ChoiceNode>(
        "FrameRate", "描画スキップの設定です", &gfx_cfg.fps_divisor, 0,
        FPS_DIVISOR_MAX,
        std::vector<std::string>{FmtLabel("おまけ"), FmtLabel("60Fps"),
                                 FmtLabel("30Fps"), FmtLabel("20Fps")},
        [&gfx_cfg] { Grp_FPSDivisor = gfx_cfg.fps_divisor; }));
  }

  ch.push_back(BuildScreenshotMenu(gfx_cfg));

#ifdef SUPPORT_GRP_API
  {
    auto api_entry = BuildApiMenu(gfx_cfg);
    api_entry->SetPoll([raw = api_entry.get()]() {
      raw->SetEnabled(GrpBackend_APICount() >= 2);
    });
    ch.push_back(std::move(api_entry));
  }
#endif

  ch.push_back(std::make_unique<SeparatorNode>());

  {
    auto msg = std::make_unique<ActionNode>(
        "MsgWindow", "ウィンドウの表示位置を決めます",
        [&gfx_cfg](MenuController &) {
          if (gfx_cfg.msg_disable) {
            gfx_cfg.msg_disable = false;
            gfx_cfg.window_upper = false;
          } else if (gfx_cfg.window_upper) {
            gfx_cfg.msg_disable = true;
            gfx_cfg.window_upper = false;
          } else {
            gfx_cfg.window_upper = true;
          }
          return true;
        },
        [&gfx_cfg](MenuController &, int delta) {
          int state = gfx_cfg.msg_disable ? 2 : (gfx_cfg.window_upper ? 0 : 1);
          state = (delta < 0) ? ((state + 2) % 3) : ((state + 1) % 3);
          gfx_cfg.window_upper = (state == 0);
          gfx_cfg.msg_disable = (state == 2);
        });
    auto *msg_raw = msg.get();
    msg->SetPoll([msg_raw, &gfx_cfg]() {
      static constexpr const char *labels[] = {"下のほう", "上のほう",
                                               "描画せず"};
      int state = gfx_cfg.msg_disable ? 2 : (gfx_cfg.window_upper ? 0 : 1);
      msg_raw->SetValue(labels[state]);
    });
    ch.push_back(std::move(msg));
  }

  return std::make_unique<EntryNode>("Graphic", "グラフィックに関する設定",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// MIDI
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildMidiMenu(AudioConfig &audio_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(2);

  auto sf_node = std::make_unique<ListNode>(
      "SoundFont", "Select a SoundFont device",
      [] { return MidBackend_DeviceCount(); },
      [](size_t i) {
        auto name = MidBackend_DeviceNameAt(i);
        auto src = MidBackend_DeviceSource(i);
        const char *label = "?";
        if (src) {
          switch (src.value()) {
          case MID_BACKEND_DEVICE_SOURCE::LOCAL:
            label = "local";
            break;
          case MID_BACKEND_DEVICE_SOURCE::SYSTEM:
            label = "system";
            break;
          case MID_BACKEND_DEVICE_SOURCE::ENV:
            label = "env";
            break;
          }
        }
        if (name) {
          return std::format("{} ({})", name.value(), label);
        }
        return std::string();
      },
      [](size_t i) -> bool {
        if (BGM_Enabled()) {
          Mid_Stop();
          MidBackend_DeviceSelect(i);
          if (auto sf = MidBackend_CurrentSoundFont()) {
            GameFlow.ctx.cfg->audio.soundfont = sf.value();
          }
          if (BGM_Playing() == BGM_PLAYING::MIDI) {
            Mid_Play();
          }
        }
        return false;
      },
      [] {
        auto cur_name = MidBackend_DeviceName();
        if (!cur_name.has_value())
          return 0;
        auto count = MidBackend_DeviceCount();
        for (size_t i = 0; i < count; i++) {
          if (auto name = MidBackend_DeviceNameAt(i);
              name && name.value() == cur_name.value())
            return static_cast<int>(i);
        }
        return 0;
      }());
  ch.push_back(std::move(sf_node));

  ch.push_back(std::make_unique<ToggleNode>(
      "SC88ProFXCompat", "Retain SC-88Pro echo on other Roland synths",
      std::ref(audio_cfg.fix_sysex_bugs), [&audio_cfg](bool on) {
        Mid_SetFlags(on ? MID_FLAGS::FIX_SYSEX_BUGS : MID_FLAGS::NONE);
      }));

  return std::make_unique<EntryNode>("MIDI", "Change MIDI playback options",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// Sound / Music
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildSoundMenu(AudioConfig &audio_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(7);

  ch.push_back(
      std::make_unique<ToggleNode>("Sound / SE", "SEを鳴らすかどうかの設定",
                                   std::ref(audio_cfg.se_enabled), [](bool on) {
                                     if (on) {
                                       sfx.LoadAll();
                                     } else {
                                       Snd_SECleanup();
                                     }
                                   }));

  ch.push_back(std::make_unique<ToggleNode>("BGM", "BGMを鳴らすかどうかの設定",
                                            std::ref(audio_cfg.bgm_enabled),
                                            [](bool on) {
                                              if (on) {
                                                if (BGM_Init()) {
                                                  track_mgr.Switch(0);
                                                }
                                              } else {
                                                BGM_Cleanup();
                                              }
                                            }));

  {
    std::vector<std::string> vol_labels;
    vol_labels.reserve(VOLUME_MAX + 1);
    for (int i = 0; i <= VOLUME_MAX; i++) {
      vol_labels.push_back(std::format("{}", i));
    }
    ch.push_back(std::make_unique<ChoiceNode>(
        "SoundVolume", "効果音の音量", &audio_cfg.se_volume, 0, VOLUME_MAX,
        std::move(vol_labels), [] { Snd_UpdateVolumes(); }));
  }

  {
    std::vector<std::string> vol_labels;
    vol_labels.reserve(VOLUME_MAX + 1);
    for (int i = 0; i <= VOLUME_MAX; i++) {
      vol_labels.push_back(std::format("{}", i));
    }
    ch.push_back(std::make_unique<ChoiceNode>(
        "BGMVolume", "音楽の音量", &audio_cfg.bgm_volume, 0, VOLUME_MAX,
        std::move(vol_labels), [] { BGM_UpdateVolume(); }));
  }

  ch.push_back(std::make_unique<ToggleNode>(
      "BGMVolNormalize", "曲ごとの音量差を補正",
      std::ref(audio_cfg.bgm_vol_norm), [](bool on) { BGM_SetGainApply(on); }));

  {
    auto packs = std::make_shared<std::vector<std::string>>();
    if (track_mgr.PacksAvailable()) {
      track_mgr.PackForeach(
          [&](std::string_view p) { packs->emplace_back(p); });
      std::ranges::sort(*packs);
    }
    auto bgm_pack = std::make_unique<ListNode>(
        "BGMPack", "BGMパックのメニューを開きます",
        [packs] { return packs->size() + 2; },
        [packs](size_t i) -> std::string {
          if (i == 0) {
            return "<使用しない>";
          }
          if (i == packs->size() + 1) {
            return "<Download>";
          }
          return (*packs)[i - 1];
        },
        [packs](size_t i) -> bool {
          if (i == 0) {
            GameFlow.ctx.cfg->audio.bgm_pack.clear();
            track_mgr.PackSet(GameFlow.ctx.cfg->audio.bgm_pack);
          } else if (i == packs->size() + 1) {
            SDL_OpenURL("https://github.com/nmlgc/BGMPacks/"
                        "releases/tag/2024-10-05");
          } else {
            GameFlow.ctx.cfg->audio.bgm_pack = (*packs)[i - 1];
            track_mgr.PackSet(GameFlow.ctx.cfg->audio.bgm_pack);
          }
          return false;
        });
    ch.push_back(std::move(bgm_pack));
  }

  ch.push_back(BuildMidiMenu(audio_cfg));

  return std::make_unique<EntryNode>("Sound / Music",
                                     "ＳＥ／ＢＧＭに関する設定", std::move(ch));
}

// ---------------------------------------------------------------------------
// JoyPad
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildPadMenu(InputConfig &input_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(4);

  static constexpr const char *kPadHelp = "パッド上のボタンを押すと変更";

  auto make_pad = [](const char *label, INPUT_PAD_BUTTON &btn) {
    auto node = std::make_unique<ActionNode>(
        label, kPadHelp, [](MenuController &) { return true; });
    auto *raw = node.get();
    raw->SetActionFn([raw, &btn](MenuController &ctrl) {
      INPUT_BITS key = ctrl.LastKey();
      key &= static_cast<INPUT_BITS>(~Pad_Data);
      if (auto temp = Key_PadSingle()) {
        btn = temp.value();
        raw->SetValue(PadButtonLabel(btn));
      }
      return !Input_IsOK(key);
    });
    raw->SetValue(PadButtonLabel(btn));
    return node;
  };

  ch.push_back(make_pad("Shot", input_cfg.pad_tama));
  ch.push_back(make_pad("Bomb", input_cfg.pad_bomb));
  ch.push_back(make_pad("SpeedDown", input_cfg.pad_shift));
  ch.push_back(make_pad("Cancel", input_cfg.pad_cancel));

  return std::make_unique<EntryNode>("Joy Pad", "パッドの設定をします",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildInputMenu(InputConfig &input_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(3);

  ch.push_back(std::make_unique<ToggleNode>(
      "Z-MessageSkip", "弾キーのメッセージスキップ設定",
      std::ref(input_cfg.z_msg_skip_enabled)));

  ch.push_back(std::make_unique<ToggleNode>(
      "Z-SpeedDown", "弾キーの押しっぱなしで低速移動",
      std::ref(input_cfg.z_spd_down_enabled)));

  ch.push_back(BuildPadMenu(input_cfg));

  return std::make_unique<EntryNode>("Input", "入力デバイスに関する設定",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// Debug (PBG_DEBUG only)
// ---------------------------------------------------------------------------
static std::unique_ptr<EntryNode> BuildDebugMenu(DebugConfig &debug_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(2);

  ch.push_back(std::make_unique<ChoiceNode>(
      "Hitbox", "[DebugMode] 弾幕判定エリア表示",
      reinterpret_cast<uint8_t *>(&debug_cfg.hitbox_display), 0, 2,
      std::vector<std::string>{FmtLabel("Off"), FmtLabel("Hit"),
                               FmtLabel("All")}));

  ch.push_back(std::make_unique<ActionNode>(
      "Bullet Gallery", "Debug bullet type gallery", [](MenuController &) {
        BulletGalleryInit();
        return true;
      }));

  return std::make_unique<EntryNode>("Debug", "デバッグ設定", std::move(ch));
}
} // namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

std::unique_ptr<IMenuNode> BuildMainMenuTree(ConfigData &cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(9);

  ch.push_back(std::make_unique<ActionNode>("Game Start", "ゲームを開始します",
                                            [](MenuController &) {
                                              GameFlow.WeaponSelectInit(false);
                                              return true;
                                            }));

  ch.push_back(std::make_unique<ActionNode>(
      "Extra Start", "ゲームを開始します(Extra)", [](MenuController &) {
        GameFlow.WeaponSelectInit(true);
        return true;
      }));

  {
    auto &files = GameFlow.ctx.ui.replay_files_storage();
    files.clear();
    SDL_EnumerateDirectory(
        ".",
        [](void *ctx, const char *, const char *name) {
          if (strstr(name, "replay_") == name && strstr(name, ".DAT")) {
            auto &f = *static_cast<std::vector<std::string> *>(ctx);
            f.emplace_back(name);
          }
          return SDL_ENUM_CONTINUE;
        },
        &files);
    std::ranges::sort(files, std::greater{});
  }
  auto replay_node = std::make_unique<ListNode>(
      "Replay", "リプレイファイルの管理",
      [] { return GameFlow.ctx.ui.replay_files_storage().size(); },
      [](size_t i) { return GameFlow.ctx.ui.replay_files_storage()[i]; },
      [](size_t i) -> bool {
        GameFlow.ctx.demos.pending_replay_file =
            GameFlow.ctx.ui.replay_files_storage()[i];
        return false;
      },
      -1, true);
  ch.push_back(std::move(replay_node));

  auto config =
      std::make_unique<EntryNode>("Config", "各種設定を変更します",
                                  std::vector<std::unique_ptr<IMenuNode>>{});
  config->AddChild(BuildDifficultyMenu(cfg.game));
  config->AddChild(BuildGraphicsMenu(cfg.graphics));
  config->AddChild(BuildSoundMenu(cfg.audio));
  config->AddChild(BuildInputMenu(cfg.input));
  ch.push_back(std::move(config));

  ch.push_back(std::make_unique<ActionNode>("Score", "スコアの表示をします",
                                            [](MenuController &) {
                                              ScoreNameInit();
                                              return true;
                                            }));

  auto music = std::make_unique<ActionNode>("Music", "音楽室に入ります",
                                            [](MenuController &) {
                                              GameFlow.ctx.ui.MsgForceClose();
                                              MusicRoomInit();
                                              return true;
                                            });
  auto *music_raw = music.get();
  music->SetPoll([music_raw]() { music_raw->SetEnabled(BGM_Enabled()); });
  ch.push_back(std::move(music));

  ch.push_back(BuildDebugMenu(cfg.debug));

  ch.push_back(std::make_unique<ActionNode>(
      "Exit", "ゲームを終了します", [](MenuController &) { return false; }));

  return std::make_unique<EntryNode>("Main Menu", "", std::move(ch));
}

} // namespace menu
