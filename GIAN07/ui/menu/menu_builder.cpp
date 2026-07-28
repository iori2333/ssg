///
/// MenuBuilder — Tree construction for the main menu and all settings
///

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <SDL3/SDL_misc.h>

#include "menu_builder.h"
#include "menu_controller.h"

#include "app/display_controller.h"
#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "audio/snd.h"
#include "audio/volume.h"
#include "data/graphics_loader.h"
#include "data/sfx_loader.h"
#include "gameplay/game_rules.h"
#include "gfx/graphics_backend.h"
#include "music/music_player.h"
#include "settings/config.h"
#include "sys/input.h"

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
      "PlayerStock", "残り人数を設定します", game_cfg.player_stock, 0,
      kMaxPlayerStock,
      std::vector<std::string>{"1", "2", "3", "4", "5", "6", "7"}));

  ch.push_back(std::make_unique<ChoiceNode>(
      "BombStock", "ボムの数を設定します", game_cfg.bomb_stock, 0,
      kMaxBombStock,
      std::vector<std::string>{"0", "1", "2", "3", "4", "5", "6"}));

  ch.push_back(std::make_unique<ChoiceNode>(
      "Difficulty", "難易度を設定します", game_cfg.game_level, GameLevel::Easy,
      GameLevel::Lunatic,
      std::vector<std::string>{FmtLabel("Easy"), FmtLabel("Normal"),
                               FmtLabel("Hard"), FmtLabel("Lunatic")}));

  ch.push_back(std::make_unique<ChoiceNode>(
      "PracticeMode", "練習模式を設定します", game_cfg.practice_mode,
      PracticeMode::Off, PracticeMode::Invincible,
      std::vector<std::string>{FmtLabel("Off"), FmtLabel("AutoB"),
                               FmtLabel("Invin")}));

  return std::make_unique<EntryNode>("Difficulty", "難易度に関する設定",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// Screenshot
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildScreenshotMenu(GraphicsConfig &gfx_cfg, DisplayController &display) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(2);

  std::vector<std::string> labels;
  labels.reserve(GRP_SCREENSHOT_EFFORT_MAX + 1);
  labels.push_back("BMP");
  for (auto i : std::views::iota(1U, GRP_SCREENSHOT_EFFORT_MAX + 1U)) {
    labels.push_back(std::format("WebP z{}", i - 1));
  }
  ch.push_back(std::make_unique<ChoiceNode>(
      "Format", "Screenshot format", gfx_cfg.screenshot_effort, 0,
      GRP_SCREENSHOT_EFFORT_MAX, std::move(labels), [&gfx_cfg, &display] {
        display.SetScreenshotEffort(gfx_cfg.screenshot_effort);
      }));

  ch.push_back(std::make_unique<SeparatorNode>());

  return std::make_unique<EntryNode>(
      "Screenshots", "Customize the screenshot format", std::move(ch));
}

// ---------------------------------------------------------------------------
// Graphics API (dynamic)
// ---------------------------------------------------------------------------

static std::unique_ptr<ListNode> BuildApiMenu(GraphicsConfig &gfx_cfg,
                                              DisplayController &display) {
  auto init_sel = static_cast<int>(GrpBackend_APIID(gfx_cfg.graphics_api));
  auto node = std::make_unique<ListNode>(
      "API", "Select rendering API", [] { return GrpBackend_APICount(); },
      [](size_t i) {
        return std::string(GrpBackend_APILabel(GrpBackend_APIString(i)));
      },
      [&gfx_cfg, &display](size_t i) {
        gfx_cfg.graphics_api = GrpBackend_APIString(i);
        (void)display.ApplyConfig(gfx_cfg);
        return false;
      },
      init_sel);
  node->BindSelection(
      [&gfx_cfg] { return GrpBackend_APIID(gfx_cfg.graphics_api); });
  return node;
}

// ---------------------------------------------------------------------------
// Graphics
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildGraphicsMenu(GraphicsConfig &gfx_cfg, UiConfig &ui_cfg,
                  DisplayController &display) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(11);

  auto dev = std::make_unique<ActionNode>(
      "Device", "", [](MenuController &) { return true; });
  dev->BindValue([&gfx_cfg] {
    return std::string(GrpBackend_DeviceLabel(gfx_cfg.device_id));
  });
  ch.push_back(std::move(dev));

  ch.push_back(std::make_unique<ChoiceNode>(
      "Display", "Switch between window and fullscreen modes",
      gfx_cfg.display_mode, DisplayMode::Windowed, DisplayMode::Fullscreen,
      std::vector<std::string>{"Window", "Fullscreen"},
      [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); }));

  {
    auto fullscreen_mode = std::make_unique<ChoiceNode>(
        "FullScr", "Fullscreen mode", gfx_cfg.fullscreen_mode,
        FullscreenMode::Borderless, FullscreenMode::Exclusive,
        std::vector<std::string>{"Borderless", "Exclusive"},
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    fullscreen_mode->BindEnabled(
        [&gfx_cfg] { return gfx_cfg.display_mode == DisplayMode::Fullscreen; });
    ch.push_back(std::move(fullscreen_mode));
  }

  {
    const auto max_scale = Grp_WindowScale4xMax();
    std::vector<std::string> labels;
    labels.reserve(max_scale + 1);
    labels.push_back("Screen");
    for (uint8_t scale = 1; scale <= max_scale; scale++) {
      labels.push_back(std::format("{:3}.{:02}x", scale / 4, (scale % 4) * 25));
    }
    auto window_scale = std::make_unique<ChoiceNode>(
        "WindowScale", "Window scaling factor", gfx_cfg.window_scale_4x, 0,
        max_scale, std::move(labels),
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    window_scale->BindEnabled(
        [&gfx_cfg] { return gfx_cfg.display_mode == DisplayMode::Windowed; });
    ch.push_back(std::move(window_scale));
  }

  {
    auto fullscreen_fit = std::make_unique<ChoiceNode>(
        "FullScrFit", "Borderless fullscreen scaling", gfx_cfg.fullscreen_fit,
        GRAPHICS_FULLSCREEN_FIT::INTEGER, GRAPHICS_FULLSCREEN_FIT::STRETCH,
        std::vector<std::string>{"Integer", "Aspect", "Stretch"},
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    fullscreen_fit->BindEnabled([&gfx_cfg] {
      return gfx_cfg.display_mode == DisplayMode::Fullscreen &&
             gfx_cfg.fullscreen_mode == FullscreenMode::Borderless;
    });
    ch.push_back(std::move(fullscreen_fit));
  }

  {
    auto scaling_mode = std::make_unique<ChoiceNode>(
        "ScaleMode", "Scaling method", gfx_cfg.scaling_mode,
        ScalingMode::Framebuffer, ScalingMode::Geometry,
        std::vector<std::string>{"FrameBuf", "Geometry"},
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    scaling_mode->BindEnabled([&gfx_cfg] {
      return gfx_cfg.display_mode != DisplayMode::Fullscreen ||
             gfx_cfg.fullscreen_mode != FullscreenMode::Exclusive;
    });
    ch.push_back(std::move(scaling_mode));
  }

  ch.push_back(std::make_unique<ChoiceNode>(
      "FrameRate", "描画スキップの設定です", gfx_cfg.fps_divisor, 0,
      kMaxFpsDivisor,
      std::vector<std::string>{FmtLabel("おまけ"), FmtLabel("60Fps"),
                               FmtLabel("30Fps"), FmtLabel("20Fps")},
      [&gfx_cfg, &display] { display.SetFrameRate(gfx_cfg.fps_divisor); }));

  ch.push_back(BuildScreenshotMenu(gfx_cfg, display));

  {
    auto api_entry = BuildApiMenu(gfx_cfg, display);
    api_entry->BindEnabled([] { return GrpBackend_APICount() >= 2; });
    ch.push_back(std::move(api_entry));
  }

  ch.push_back(std::make_unique<SeparatorNode>());

  ch.push_back(std::make_unique<ChoiceNode>(
      "MsgWindow", "ウィンドウの表示位置を決めます", ui_cfg.message_window,
      MessageWindowMode::Upper, MessageWindowMode::Hidden,
      std::vector<std::string>{"上のほう", "下のほう", "描画せず"}));

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
      [&audio_cfg](size_t i) -> bool {
        if (BGM_Enabled()) {
          Mid_Stop();
          MidBackend_DeviceSelect(i);
          if (auto sf = MidBackend_CurrentSoundFont()) {
            audio_cfg.soundfont = sf.value();
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
        (void)Mid_SetFlags(on ? MID_FLAGS::FIX_SYSEX_BUGS : MID_FLAGS::NONE);
      }));

  return std::make_unique<EntryNode>("MIDI", "Change MIDI playback options",
                                     std::move(ch));
}

// ---------------------------------------------------------------------------
// Sound / Music
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode> BuildSoundMenu(AudioConfig &audio_cfg,
                                                 data::SfxLoader &sound_effects,
                                                 MusicPlayer &music) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(7);

  ch.push_back(std::make_unique<ToggleNode>(
      "Sound / SE", "SEを鳴らすかどうかの設定", std::ref(audio_cfg.se_enabled),
      [&audio_cfg, &sound_effects](bool on) {
        if (on) {
          if (!sound_effects.Load()) {
            audio_cfg.se_enabled = false;
          }
        } else {
          Snd_SECleanup();
        }
      }));

  ch.push_back(std::make_unique<ToggleNode>("BGM", "BGMを鳴らすかどうかの設定",
                                            std::ref(audio_cfg.bgm_enabled),
                                            [&music](bool on) {
                                              if (on) {
                                                if (BGM_Init()) {
                                                  music.Play(0);
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
        "SoundVolume", "効果音の音量", audio_cfg.se_volume, 0, VOLUME_MAX,
        std::move(vol_labels), [&audio_cfg] {
          Snd_SetVolumes(audio_cfg.bgm_volume, audio_cfg.se_volume);
        }));
  }

  {
    std::vector<std::string> vol_labels;
    vol_labels.reserve(VOLUME_MAX + 1);
    for (int i = 0; i <= VOLUME_MAX; i++) {
      vol_labels.push_back(std::format("{}", i));
    }
    ch.push_back(std::make_unique<ChoiceNode>(
        "BGMVolume", "音楽の音量", audio_cfg.bgm_volume, 0, VOLUME_MAX,
        std::move(vol_labels), [&audio_cfg] {
          Mid_SetVolume(audio_cfg.bgm_volume);
          Snd_SetVolumes(audio_cfg.bgm_volume, audio_cfg.se_volume);
        }));
  }

  ch.push_back(std::make_unique<ToggleNode>(
      "BGMVolNormalize", "曲ごとの音量差を補正",
      std::ref(audio_cfg.bgm_vol_norm), [](bool on) { BGM_SetGainApply(on); }));

  {
    auto packs = std::make_shared<std::vector<std::string>>();
    if (music.HasPacks()) {
      music.ForEachPack([&](std::string_view p) { packs->emplace_back(p); });
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
        [packs, &audio_cfg, &music](size_t i) -> bool {
          if (i == 0) {
            audio_cfg.bgm_pack.clear();
            music.SetPack(audio_cfg.bgm_pack);
          } else if (i == packs->size() + 1) {
            SDL_OpenURL("https://github.com/nmlgc/BGMPacks/"
                        "releases/tag/2024-10-05");
          } else {
            audio_cfg.bgm_pack = (*packs)[i - 1];
            music.SetPack(audio_cfg.bgm_pack);
          }
          return false;
        });
    bgm_pack->BindSelection([packs, &audio_cfg] {
      if (audio_cfg.bgm_pack.empty()) {
        return 0;
      }
      const auto selected =
          std::ranges::find(*packs, audio_cfg.bgm_pack) - packs->begin();
      return selected < static_cast<std::ptrdiff_t>(packs->size())
                 ? static_cast<int>(selected + 1)
                 : 0;
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

static void ApplyPadBindings(const InputConfig &input_cfg) {
  const std::array bindings = {
      INPUT_PAD_BINDING{input_cfg.pad_tama, KEY_TAMA},
      INPUT_PAD_BINDING{input_cfg.pad_bomb, KEY_BOMB},
      INPUT_PAD_BINDING{input_cfg.pad_shift, KEY_SHIFT},
      INPUT_PAD_BINDING{input_cfg.pad_cancel, KEY_ESC},
  };
  Key_SetPadBindings(bindings);
}

static std::unique_ptr<EntryNode> BuildPadMenu(InputConfig &input_cfg) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(4);

  static constexpr const char *kPadHelp = "パッド上のボタンを押すと変更";

  auto make_pad = [&input_cfg](const char *label, INPUT_PAD_BUTTON &btn) {
    auto node = std::make_unique<ActionNode>(
        label, kPadHelp, [&btn, &input_cfg](MenuController &ctrl) {
          INPUT_BITS key = ctrl.LastKey();
          key &= static_cast<INPUT_BITS>(~Pad_Data);
          if (auto temp = Key_PadSingle()) {
            btn = temp.value();
            ApplyPadBindings(input_cfg);
          }
          return !Input_IsOK(key);
        });
    node->BindValue([&btn] { return PadButtonLabel(btn); });
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
static std::unique_ptr<EntryNode>
BuildDebugMenu(DebugConfig &debug_cfg,
               std::function<void(MainMenuAction)> on_action) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(2);

  ch.push_back(std::make_unique<ChoiceNode>(
      "Hitbox", "[DebugMode] 弾幕判定エリア表示", debug_cfg.hitbox_display, 0,
      2,
      std::vector<std::string>{FmtLabel("Off"), FmtLabel("Hit"),
                               FmtLabel("All")}));

  ch.push_back(std::make_unique<ActionNode>(
      "Bullet Gallery", "Debug bullet type gallery",
      [on_action](MenuController &) {
        on_action(MainMenuAction::OpenBulletGallery);
        return true;
      }));

  return std::make_unique<EntryNode>("Debug", "デバッグ設定", std::move(ch));
}
} // namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

std::unique_ptr<IMenuNode>
BuildMainMenuTree(ConfigData &cfg, MainMenuServices services,
                  std::function<void(MainMenuAction)> on_action) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(9);

  ch.push_back(std::make_unique<ActionNode>(
      "Game Start", "ゲームを開始します", [on_action](MenuController &) {
        on_action(MainMenuAction::StartGame);
        return true;
      }));

  ch.push_back(
      std::make_unique<ActionNode>("Extra Start", "ゲームを開始します(Extra)",
                                   [on_action](MenuController &) {
                                     on_action(MainMenuAction::StartExtra);
                                     return true;
                                   }));

  ch.push_back(std::make_unique<ActionNode>(
      "Replay", "リプレイファイルの管理", [on_action](MenuController &) {
        on_action(MainMenuAction::OpenReplay);
        return true;
      }));

  auto config =
      std::make_unique<EntryNode>("Config", "各種設定を変更します",
                                  std::vector<std::unique_ptr<IMenuNode>>{});
  config->AddChild(BuildDifficultyMenu(cfg.game));
  config->AddChild(BuildGraphicsMenu(cfg.graphics, cfg.ui, services.display));
  config->AddChild(
      BuildSoundMenu(cfg.audio, services.sound_effects, services.music));
  config->AddChild(BuildInputMenu(cfg.input));
  ch.push_back(std::move(config));

  ch.push_back(std::make_unique<ActionNode>(
      "Score", "スコアの表示をします", [on_action](MenuController &) {
        on_action(MainMenuAction::OpenScore);
        return true;
      }));

  auto music = std::make_unique<ActionNode>(
      "Music", "音楽室に入ります", [on_action](MenuController &) {
        on_action(MainMenuAction::OpenMusicRoom);
        return true;
      });
  music->BindEnabled([] { return BGM_Enabled(); });
  ch.push_back(std::move(music));

  ch.push_back(BuildDebugMenu(cfg.debug, on_action));

  ch.push_back(std::make_unique<ActionNode>(
      "Exit", "ゲームを終了します", [](MenuController &) { return false; }));

  return std::make_unique<EntryNode>("Main Menu", "", std::move(ch));
}

} // namespace menu
