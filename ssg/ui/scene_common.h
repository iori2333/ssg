///
/// Shared free helpers for UI scenes, so scene classes do not duplicate
/// i18n-key lookups, stage/player labels, or common text rendering.
///

#pragma once

#include <string_view>

enum class StageId : uint8_t;
enum class PlayerType : uint8_t;

#include "gfx/core/coords.h"
#include "gfx/core/pixelformat.h"
#include "gfx/text/text.h"
#include "gfx/text/text_renderer.h"
#include "i18n/localization.h"
#include "ui/menu/menu_tree.h"

namespace ui {

// Resolves [key] through the localization catalog.
[[nodiscard]] std::string_view Text(const i18n::Localization &localization,
                                    std::string_view key);

// Localized display name for a stage.
[[nodiscard]] std::string_view StageName(const i18n::Localization &localization,
                                         StageId stage);

// Localized display name for a player type.
[[nodiscard]] std::string_view PlayerName(const i18n::Localization &localization,
                                          PlayerType player);

// Renders a line of UI text with a drop shadow, centered horizontally when
// requested. Shared by the score and replay scenes.
void RenderUiText(PixelPoint position, TextRenderRectId rect,
                  std::string_view text, bool centered = false);

// Builds a lazily-resolved MenuText for [key]. Shared by menu builders.
[[nodiscard]] menu::MenuText Localized(i18n::Localization &localization,
                                       std::string_view key);

} // namespace ui