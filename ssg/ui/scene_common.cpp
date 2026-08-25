///
/// Shared free helpers for UI scenes.
///

#include <array>

#include "scene_common.h"

#include "gameplay/game_rules.h"
#include "player/loadout/player_loadout.h"

namespace ui {

std::string_view Text(const i18n::Localization &localization,
                      std::string_view key) {
  return localization.Text(i18n::TextIdFromKey(key));
}

std::string_view StageName(const i18n::Localization &localization,
                           StageId stage) {
  constexpr std::array keys = {"ui.value.stage1", "ui.value.stage2",
                               "ui.value.stage3", "ui.value.stage4",
                               "ui.value.stage5", "ui.value.stage6",
                               "ui.value.extra"};
  const auto index = std::to_underlying(stage);
  return index < keys.size() ? Text(localization, keys[index])
                             : std::string_view{};
}

std::string_view PlayerName(const i18n::Localization &localization,
                            PlayerType player) {
  constexpr std::array keys = {"ui.value.wide", "ui.value.homing",
                               "ui.value.laser"};
  const auto index = std::to_underlying(player);
  return index < keys.size() ? Text(localization, keys[index])
                             : std::string_view{};
}

void RenderUiText(PixelPoint position, TextRenderRectId rect,
                  std::string_view text, bool centered) {
  TextRenderer().Render(
      position, rect, text, [text, centered](TextRenderSession &s) {
        s.SetFont(FontId::Normal);
        const auto x = centered ? TextLayoutXCenter(s, text) : 0;
        s.Put({.x = x + 1, .y = 1}, text, Rgb{.r = 96, .g = 96, .b = 96});
        s.Put({.x = x, .y = 0}, text, Rgb{.r = 255, .g = 255, .b = 255});
      });
}

menu::MenuText Localized(i18n::Localization &localization,
                         std::string_view key) {
  const auto id = i18n::TextIdFromKey(key);
  return {[&localization, id] { return localization.Text(id); }};
}

} // namespace ui