///
/// MsgWindow - Message window processing
///

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "gfx/text.h"
#include "util/enum_flags.h"

// Message window flags

enum class MsgWindowFlags : uint8_t {
  None = 0x0,
  WithFace = 0x1, // Pads all text to leave room for a face portrait.
  Center = 0x2,   // Horizontally centers all text.
};

template <> inline constexpr bool util::EnableEnumFlags<MsgWindowFlags> = true;

class MsgWindow {
public:
  void Init(const WindowLtrb &rc, MsgWindowFlags flags = MsgWindowFlags::None);
  void Open();       // Open the message window
  void Close();      // Close the message window
  void ForceClose(); // Force close the message window
  void Tick();       // Run message window logic
  void Draw();       // Draw the message window
  void AppendMessage(std::string_view message);
  void SetFace(uint8_t face_id);
  void SetFont(FontId font);
  void NewPage();
  void ShowHelp(std::string_view help);

private:
  static constexpr auto kMessageLines = 5;

  void MsgBlank(); // Clear strings and reset to first line

  WindowLtrb max_size{}; // Final window size
  WindowLtrb now_size{}; // Current window size
  PixelPoint text_topleft{};

  MsgWindowFlags flags{};
  FontId font_id{};   // Font to use
  uint8_t font_dy{};  // Font Y increment
  uint8_t state{};    // State
  uint8_t max_line{}; // Max displayable lines
  uint8_t line{};     // Next line index

  uint8_t face_id{};    // Current face ID
  uint8_t next_face{};  // Next face ID to show
  uint8_t face_state{}; // Face state
  uint8_t face_time{};  // Face display counter

  std::array<std::string_view, kMessageLines> msg{}; // Displayed messages

  // Contains all text from [msg], concatenated with '\n'.
  std::string text;

  std::optional<TextRenderRectId> trr;
};
