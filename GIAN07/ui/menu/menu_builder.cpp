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
#include "audio/audio_system.h"
#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/midi_backend.h"
#include "audio/volume.h"
#include "gameplay/game_rules.h"
#include "gfx/graphics_backend.h"
#include "i18n/localization.h"
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

static MenuText Localized(i18n::Localization &localization,
                          std::string_view key) {
  const auto id = i18n::TextIdFromKey(key);
  return MenuText([&localization, id] { return localization.Text(id); });
}

static ChoiceLabels
LocalizedLabels(i18n::Localization &localization,
                std::initializer_list<std::string_view> keys) {
  std::vector<MenuText> labels;
  labels.reserve(keys.size());
  for (const auto key : keys) {
    labels.push_back(Localized(localization, key));
  }
  return ChoiceLabels(std::move(labels));
}

static void LocalizeToggleValues(ToggleNode &node,
                                 i18n::Localization &localization) {
  node.SetValueText(Localized(localization, "ui.common.on"),
                    Localized(localization, "ui.common.off"));
}

static std::string LanguageLabel(std::string_view language) {
  if (language == "ja") {
    return "日本語";
  }
  if (language == "en") {
    return "English";
  }
  if (language == "zh") {
    return "简体中文";
  }
  return std::string(language);
}

static std::unique_ptr<ListNode>
BuildLanguageMenu(UiConfig &ui_cfg, i18n::Localization &localization) {
  auto node = std::make_unique<ListNode>(
      Localized(localization, "ui.menu.language.title"),
      Localized(localization, "ui.menu.language.help"),
      [&localization] { return localization.LanguageCount(); },
      [&localization](size_t index) {
        return LanguageLabel(localization.LanguageAt(index));
      },
      [&ui_cfg, &localization](size_t index) {
        const auto language = localization.LanguageAt(index);
        if (!localization.SetLanguage(language)) {
          return true;
        }
        ui_cfg.language = language;
        return false;
      },
      static_cast<int>(localization.CurrentLanguageIndex()));
  node->BindSelection(
      [&localization] { return localization.CurrentLanguageIndex(); });
  return node;
}

// ---------------------------------------------------------------------------
// Difficulty
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildDifficultyMenu(GameConfig &game_cfg, i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(4);

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.player_stock.title"),
      Localized(localization, "ui.menu.player_stock.help"),
      game_cfg.player_stock, 0, kMaxPlayerStock,
      std::vector<std::string>{"1", "2", "3", "4", "5", "6", "7"}));

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.bomb_stock.title"),
      Localized(localization, "ui.menu.bomb_stock.help"), game_cfg.bomb_stock,
      0, kMaxBombStock,
      std::vector<std::string>{"0", "1", "2", "3", "4", "5", "6"}));

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.practice.title"),
      Localized(localization, "ui.menu.practice.help"), game_cfg.practice_mode,
      PracticeMode::Off, PracticeMode::Invincible,
      LocalizedLabels(localization, {"ui.common.off", "ui.value.auto_bomb",
                                     "ui.value.invincible"})));

  auto focus_hitbox = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.focus_hitbox.title"),
      Localized(localization, "ui.menu.focus_hitbox.help"),
      std::ref(game_cfg.show_focus_hitbox));
  LocalizeToggleValues(*focus_hitbox, localization);
  ch.push_back(std::move(focus_hitbox));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.difficulty.title"),
      Localized(localization, "ui.menu.difficulty.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// Screenshot
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildScreenshotMenu(GraphicsConfig &gfx_cfg, DisplayController &display,
                    i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(2);

  std::vector<std::string> labels;
  labels.reserve(kScreenshotEffortMax + 1);
  labels.push_back("BMP");
  for (auto i : std::views::iota(1U, kScreenshotEffortMax + 1U)) {
    labels.push_back(std::format("WebP z{}", i - 1));
  }
  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.screenshot_format.title"),
      Localized(localization, "ui.menu.screenshot_format.help"),
      gfx_cfg.screenshot_effort, 0, kScreenshotEffortMax, std::move(labels),
      [&gfx_cfg, &display] {
        display.SetScreenshotEffort(gfx_cfg.screenshot_effort);
      }));

  ch.push_back(std::make_unique<SeparatorNode>());

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.screenshots.title"),
      Localized(localization, "ui.menu.screenshots.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// Graphics API (dynamic)
// ---------------------------------------------------------------------------

static std::unique_ptr<ListNode>
BuildApiMenu(GraphicsConfig &gfx_cfg, DisplayController &display,
             i18n::Localization &localization) {
  auto init_sel = static_cast<int>(GrpBackend_APIID(gfx_cfg.graphics_api));
  auto node = std::make_unique<ListNode>(
      Localized(localization, "ui.menu.api.title"),
      Localized(localization, "ui.menu.api.help"),
      [] { return GrpBackend_APICount(); },
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
                  DisplayController &display,
                  i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(10);

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.display.title"),
      Localized(localization, "ui.menu.display.help"), gfx_cfg.display_mode,
      DisplayMode::Windowed, DisplayMode::Fullscreen,
      LocalizedLabels(localization,
                      {"ui.value.windowed", "ui.value.fullscreen"}),
      [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); }));

  {
    auto fullscreen_mode = std::make_unique<ChoiceNode>(
        Localized(localization, "ui.menu.fullscreen_mode.title"),
        Localized(localization, "ui.menu.fullscreen_mode.help"),
        gfx_cfg.fullscreen_mode, FullscreenMode::Borderless,
        FullscreenMode::Exclusive,
        LocalizedLabels(localization,
                        {"ui.value.borderless", "ui.value.exclusive"}),
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    fullscreen_mode->BindEnabled(
        [&gfx_cfg] { return gfx_cfg.display_mode == DisplayMode::Fullscreen; });
    ch.push_back(std::move(fullscreen_mode));
  }

  {
    const auto max_scale = Grp_WindowScale4xMax();
    std::vector<MenuText> labels;
    labels.reserve(max_scale + 1);
    labels.push_back(Localized(localization, "ui.value.screen"));
    for (uint8_t scale = 1; scale <= max_scale; scale++) {
      labels.emplace_back(
          std::format("{:3}.{:02}x", scale / 4, (scale % 4) * 25));
    }
    auto window_scale = std::make_unique<ChoiceNode>(
        Localized(localization, "ui.menu.window_scale.title"),
        Localized(localization, "ui.menu.window_scale.help"),
        gfx_cfg.window_scale_4x, 0, max_scale, ChoiceLabels(std::move(labels)),
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    window_scale->BindEnabled(
        [&gfx_cfg] { return gfx_cfg.display_mode == DisplayMode::Windowed; });
    ch.push_back(std::move(window_scale));
  }

  {
    auto fullscreen_fit = std::make_unique<ChoiceNode>(
        Localized(localization, "ui.menu.fullscreen_fit.title"),
        Localized(localization, "ui.menu.fullscreen_fit.help"),
        gfx_cfg.fullscreen_fit, GRAPHICS_FULLSCREEN_FIT::INTEGER,
        GRAPHICS_FULLSCREEN_FIT::STRETCH,
        LocalizedLabels(localization, {"ui.value.integer", "ui.value.aspect",
                                       "ui.value.stretch"}),
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    fullscreen_fit->BindEnabled([&gfx_cfg] {
      return gfx_cfg.display_mode == DisplayMode::Fullscreen &&
             gfx_cfg.fullscreen_mode == FullscreenMode::Borderless;
    });
    ch.push_back(std::move(fullscreen_fit));
  }

  {
    auto scaling_mode = std::make_unique<ChoiceNode>(
        Localized(localization, "ui.menu.scaling_mode.title"),
        Localized(localization, "ui.menu.scaling_mode.help"),
        gfx_cfg.scaling_mode, ScalingMode::Framebuffer, ScalingMode::Geometry,
        LocalizedLabels(localization,
                        {"ui.value.framebuffer", "ui.value.geometry"}),
        [&gfx_cfg, &display] { (void)display.ApplyConfig(gfx_cfg); });
    scaling_mode->BindEnabled([&gfx_cfg] {
      return gfx_cfg.display_mode != DisplayMode::Fullscreen ||
             gfx_cfg.fullscreen_mode != FullscreenMode::Exclusive;
    });
    ch.push_back(std::move(scaling_mode));
  }

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.frame_rate.title"),
      Localized(localization, "ui.menu.frame_rate.help"), gfx_cfg.fps_divisor,
      0, kMaxFpsDivisor,
      LocalizedLabels(localization, {"ui.value.bonus", "ui.value.60fps",
                                     "ui.value.30fps", "ui.value.20fps"}),
      [&gfx_cfg, &display] { display.SetFrameRate(gfx_cfg.fps_divisor); }));

  ch.push_back(BuildScreenshotMenu(gfx_cfg, display, localization));

  {
    auto api_entry = BuildApiMenu(gfx_cfg, display, localization);
    api_entry->BindEnabled([] { return GrpBackend_APICount() >= 2; });
    ch.push_back(std::move(api_entry));
  }

  ch.push_back(std::make_unique<SeparatorNode>());

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.message_window.title"),
      Localized(localization, "ui.menu.message_window.help"),
      ui_cfg.message_window, MessageWindowMode::Upper,
      MessageWindowMode::Hidden,
      LocalizedLabels(localization, {"ui.value.upper", "ui.value.lower",
                                     "ui.value.hidden"})));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.graphics.title"),
      Localized(localization, "ui.menu.graphics.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// MIDI
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildMidiMenu(AudioConfig &audio_cfg, MusicPlayer &music,
              i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(3);

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.midi_variant.title"),
      Localized(localization, "ui.menu.midi_variant.help"),
      audio_cfg.midi_variant, MidiVariant::Original, MidiVariant::Arranged,
      LocalizedLabels(localization, {"ui.value.original", "ui.value.arranged"}),
      [&audio_cfg, &music] { music.SetMidiVariant(audio_cfg.midi_variant); }));

  auto sf_node = std::make_unique<ListNode>(
      Localized(localization, "ui.menu.soundfont.title"),
      Localized(localization, "ui.menu.soundfont.help"),
      [] { return MidiBackendDeviceCount(); },
      [](size_t i) {
        auto name = MidiBackendDeviceNameAt(i);
        auto src = MidiBackendDeviceSource(i);
        const char *label = "?";
        if (src) {
          switch (src.value()) {
          case MidiDeviceSource::Local:
            label = "local";
            break;
          case MidiDeviceSource::System:
            label = "system";
            break;
          case MidiDeviceSource::Environment:
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
        if (BgmIsEnabled()) {
          MidiStop();
          MidiBackendSelectDevice(i);
          if (auto sf = MidiBackendCurrentSoundFont()) {
            audio_cfg.soundfont = sf.value();
          }
          if (BgmPlayingSource() == BgmPlaybackSource::Midi) {
            MidiPlay();
          }
        }
        return false;
      },
      [] {
        auto cur_name = MidiBackendDeviceName();
        if (!cur_name.has_value())
          return 0;
        auto count = MidiBackendDeviceCount();
        for (size_t i = 0; i < count; i++) {
          if (auto name = MidiBackendDeviceNameAt(i);
              name && name.value() == cur_name.value())
            return static_cast<int>(i);
        }
        return 0;
      }());
  ch.push_back(std::move(sf_node));

  auto compat = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.sc88_compat.title"),
      Localized(localization, "ui.menu.sc88_compat.help"),
      std::ref(audio_cfg.fix_sysex_bugs), [&audio_cfg](bool on) {
        (void)MidiSetFlags(on ? MidiFlags::FixSysExBugs : MidiFlags::None);
      });
  LocalizeToggleValues(*compat, localization);
  ch.push_back(std::move(compat));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.midi.title"),
      Localized(localization, "ui.menu.midi.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// Sound / Music
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildSoundMenu(AudioConfig &audio_cfg, AudioSystem &audio, MusicPlayer &music,
               i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(7);

  auto sound_enabled = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.sound_effects.title"),
      Localized(localization, "ui.menu.sound_effects.help"),
      std::ref(audio_cfg.se_enabled), [&audio_cfg, &audio](bool on) {
        if (!audio.EnableSfx(on)) {
          audio_cfg.se_enabled = false;
        }
      });
  LocalizeToggleValues(*sound_enabled, localization);
  ch.push_back(std::move(sound_enabled));

  auto bgm_enabled = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.bgm.title"),
      Localized(localization, "ui.menu.bgm.help"),
      std::ref(audio_cfg.bgm_enabled), [&audio_cfg, &audio](bool on) {
        if (!audio.EnableBgm(on, audio_cfg.soundfont)) {
          audio_cfg.bgm_enabled = false;
        }
      });
  LocalizeToggleValues(*bgm_enabled, localization);
  ch.push_back(std::move(bgm_enabled));

  {
    std::vector<std::string> vol_labels;
    vol_labels.reserve(kMaxAudioVolume + 1);
    for (int i = 0; i <= kMaxAudioVolume; i++) {
      vol_labels.push_back(std::format("{}", i));
    }
    ch.push_back(std::make_unique<ChoiceNode>(
        Localized(localization, "ui.menu.sound_volume.title"),
        Localized(localization, "ui.menu.sound_volume.help"),
        audio_cfg.se_volume, 0, kMaxAudioVolume, std::move(vol_labels),
        [&audio_cfg, &audio] {
          audio.SetVolumes(audio_cfg.bgm_volume, audio_cfg.se_volume);
        }));
  }

  {
    std::vector<std::string> vol_labels;
    vol_labels.reserve(kMaxAudioVolume + 1);
    for (int i = 0; i <= kMaxAudioVolume; i++) {
      vol_labels.push_back(std::format("{}", i));
    }
    ch.push_back(std::make_unique<ChoiceNode>(
        Localized(localization, "ui.menu.bgm_volume.title"),
        Localized(localization, "ui.menu.bgm_volume.help"),
        audio_cfg.bgm_volume, 0, kMaxAudioVolume, std::move(vol_labels),
        [&audio_cfg, &audio] {
          audio.SetVolumes(audio_cfg.bgm_volume, audio_cfg.se_volume);
        }));
  }

  auto normalize = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.bgm_normalize.title"),
      Localized(localization, "ui.menu.bgm_normalize.help"),
      std::ref(audio_cfg.bgm_vol_norm),
      [&audio](bool on) { audio.SetNormalization(on); });
  LocalizeToggleValues(*normalize, localization);
  ch.push_back(std::move(normalize));

  {
    auto packs = std::make_shared<std::vector<std::string>>();
    if (music.HasPacks()) {
      music.ForEachPack([&](std::string_view p) { packs->emplace_back(p); });
      std::ranges::sort(*packs);
    }
    auto bgm_pack = std::make_unique<ListNode>(
        Localized(localization, "ui.menu.bgm_pack.title"),
        Localized(localization, "ui.menu.bgm_pack.help"),
        [packs] { return packs->size() + 2; },
        [packs, &localization](size_t i) -> std::string {
          if (i == 0) {
            return std::format("<{}>", localization.Text(i18n::TextIdFromKey(
                                           "ui.value.none")));
          }
          if (i == packs->size() + 1) {
            return std::format("<{}>", localization.Text(i18n::TextIdFromKey(
                                           "ui.value.download")));
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

  ch.push_back(BuildMidiMenu(audio_cfg, music, localization));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.sound.title"),
      Localized(localization, "ui.menu.sound.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// JoyPad
// ---------------------------------------------------------------------------

static void ApplyPadBindings(InputSystem &input, const InputConfig &input_cfg) {
  const std::array bindings = {
      INPUT_PAD_BINDING{input_cfg.pad_tama, KEY_TAMA},
      INPUT_PAD_BINDING{input_cfg.pad_bomb, KEY_BOMB},
      INPUT_PAD_BINDING{input_cfg.pad_shift, KEY_SHIFT},
      INPUT_PAD_BINDING{input_cfg.pad_cancel, KEY_ESC},
  };
  input.SetPadBindings(bindings);
}

static std::unique_ptr<EntryNode>
BuildPadMenu(InputConfig &input_cfg, InputSystem &input,
             i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(4);

  auto make_pad = [&input_cfg, &input, &localization](
                      std::string_view label_key, INPUT_PAD_BUTTON &btn) {
    auto node = std::make_unique<ActionNode>(
        Localized(localization, label_key),
        Localized(localization, "ui.menu.pad_binding.help"),
        [&btn, &input_cfg, &input](MenuController &ctrl) {
          INPUT_BITS key = ctrl.LastKey();
          key &= static_cast<INPUT_BITS>(~input.Current().pad);
          if (auto temp = input.PadSingle()) {
            btn = temp.value();
            ApplyPadBindings(input, input_cfg);
          }
          return !Input_IsOK(key);
        });
    node->BindValue([&btn] { return PadButtonLabel(btn); });
    return node;
  };

  ch.push_back(make_pad("ui.menu.pad_shot.title", input_cfg.pad_tama));
  ch.push_back(make_pad("ui.menu.pad_bomb.title", input_cfg.pad_bomb));
  ch.push_back(make_pad("ui.menu.pad_slow.title", input_cfg.pad_shift));
  ch.push_back(make_pad("ui.menu.pad_cancel.title", input_cfg.pad_cancel));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.joypad.title"),
      Localized(localization, "ui.menu.joypad.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

static std::unique_ptr<EntryNode>
BuildInputMenu(InputConfig &input_cfg, InputSystem &input,
               i18n::Localization &localization) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(3);

  auto message_skip = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.message_skip.title"),
      Localized(localization, "ui.menu.message_skip.help"),
      std::ref(input_cfg.z_msg_skip_enabled));
  LocalizeToggleValues(*message_skip, localization);
  ch.push_back(std::move(message_skip));

  auto speed_down = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.speed_down.title"),
      Localized(localization, "ui.menu.speed_down.help"),
      std::ref(input_cfg.z_spd_down_enabled));
  LocalizeToggleValues(*speed_down, localization);
  ch.push_back(std::move(speed_down));

  ch.push_back(BuildPadMenu(input_cfg, input, localization));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.input.title"),
      Localized(localization, "ui.menu.input.help"), std::move(ch));
}

// ---------------------------------------------------------------------------
// Debug (PBG_DEBUG only)
// ---------------------------------------------------------------------------
static std::unique_ptr<EntryNode>
BuildDebugMenu(DebugConfig &debug_cfg, i18n::Localization &localization,
               std::function<void(MainMenuAction)> on_action) {
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(3);

  ch.push_back(std::make_unique<ChoiceNode>(
      Localized(localization, "ui.menu.hitbox.title"),
      Localized(localization, "ui.menu.hitbox.help"), debug_cfg.hitbox_display,
      0, 2,
      LocalizedLabels(localization,
                      {"ui.common.off", "ui.value.hit", "ui.value.all"})));

  auto demo_recording = std::make_unique<ToggleNode>(
      Localized(localization, "ui.menu.demo_recording.title"),
      Localized(localization, "ui.menu.demo_recording.help"),
      std::ref(debug_cfg.demo_recording));
  LocalizeToggleValues(*demo_recording, localization);
  ch.push_back(std::move(demo_recording));

  ch.push_back(std::make_unique<ActionNode>(
      Localized(localization, "ui.menu.bullet_gallery.title"),
      Localized(localization, "ui.menu.bullet_gallery.help"),
      [on_action](MenuController &) {
        on_action(MainMenuAction::OpenBulletGallery);
        return true;
      }));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.debug.title"),
      Localized(localization, "ui.menu.debug.help"), std::move(ch));
}
} // namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

std::unique_ptr<IMenuNode>
BuildMainMenuTree(ConfigData &cfg, MainMenuServices services,
                  std::function<void(MainMenuAction)> on_action) {
  auto &localization = services.localization;
  std::vector<std::unique_ptr<IMenuNode>> ch;
  ch.reserve(9);

  ch.push_back(std::make_unique<ActionNode>(
      Localized(localization, "ui.menu.game_start.title"),
      Localized(localization, "ui.menu.game_start.help"),
      [on_action](MenuController &) {
        on_action(MainMenuAction::StartGame);
        return true;
      }));

  auto extra_start = std::make_unique<ActionNode>(
      Localized(localization, "ui.menu.extra_start.title"),
      Localized(localization, "ui.menu.extra_start.help"),
      [on_action](MenuController &) {
        on_action(MainMenuAction::StartExtra);
        return true;
      });
  extra_start->BindEnabled(
      [&cfg] { return cfg.progress.extra_stg_flags != 0; });
  ch.push_back(std::move(extra_start));

  ch.push_back(std::make_unique<ActionNode>(
      Localized(localization, "ui.menu.replay.title"),
      Localized(localization, "ui.menu.replay.help"),
      [on_action](MenuController &) {
        on_action(MainMenuAction::OpenReplay);
        return true;
      }));

  auto config = std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.config.title"),
      Localized(localization, "ui.menu.config.help"),
      std::vector<std::unique_ptr<IMenuNode>>{});
  config->AddChild(BuildDifficultyMenu(cfg.game, localization));
  config->AddChild(BuildLanguageMenu(cfg.ui, services.localization));
  config->AddChild(
      BuildGraphicsMenu(cfg.graphics, cfg.ui, services.display, localization));
  config->AddChild(
      BuildSoundMenu(cfg.audio, services.audio, services.music, localization));
  config->AddChild(BuildInputMenu(cfg.input, services.input, localization));
  ch.push_back(std::move(config));

  ch.push_back(std::make_unique<ActionNode>(
      Localized(localization, "ui.menu.score.title"),
      Localized(localization, "ui.menu.score.help"),
      [on_action](MenuController &) {
        on_action(MainMenuAction::OpenScore);
        return true;
      }));

  auto music = std::make_unique<ActionNode>(
      Localized(localization, "ui.menu.music.title"),
      Localized(localization, "ui.menu.music.help"),
      [on_action](MenuController &) {
        on_action(MainMenuAction::OpenMusicRoom);
        return true;
      });
  music->BindEnabled([] { return BgmIsEnabled(); });
  ch.push_back(std::move(music));

  ch.push_back(BuildDebugMenu(cfg.debug, localization, on_action));

  ch.push_back(
      std::make_unique<ActionNode>(Localized(localization, "ui.common.exit"),
                                   Localized(localization, "ui.menu.exit.help"),
                                   [](MenuController &) { return false; }));

  return std::make_unique<EntryNode>(
      Localized(localization, "ui.menu.main.title"), "", std::move(ch));
}

} // namespace menu
