///
/// MsgWindow - Message window processing
///

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "gfx/text.h"

// Message window flags
enum class MsgWindowFlags : uint8_t {
  NONE = 0x0,
  WITH_FACE = 0x1, // Pads all text to leave room for a face portrait.
  CENTER = 0x2,    // Horizontally centers all text.
  HAS_BITFLAG_OPERATORS = 3,
};

class MsgWindow {
public:
  void Init(const WINDOW_LTRB &rc, MsgWindowFlags flags = MsgWindowFlags::NONE);
  void Open();       // Open the message window
  void Close();      // Close the message window
  void ForceClose(); // Force close the message window
  void Tick();       // Run message window logic
  void Draw();       // Draw the message window
  void AppendMessage(std::string_view message);
  void SetFace(uint8_t face_id);
  void SetFont(FONT_ID font);
  void NewPage();
  void ShowHelp(std::string_view help);

private:
  static constexpr auto kMessageLines = 5;

  void MsgBlank(); // Clear strings and reset to first line

  WINDOW_LTRB max_size{}; // Final window size
  WINDOW_LTRB now_size{}; // Current window size
  PIXEL_POINT text_topleft{};

  MsgWindowFlags flags{};
  FONT_ID font_id{};  // Font to use
  uint8_t font_dy{};  // Font Y increment
  uint8_t state{};    // State
  uint8_t max_line{}; // Max displayable lines
  uint8_t line{};     // Next line index

  uint8_t face_id{};    // Current face ID
  uint8_t next_face{};  // Next face ID to show
  uint8_t face_state{}; // Face state
  uint8_t face_time{};  // Face display counter

  std::string_view msg[kMessageLines]{}; // Pointers to displayed messages

  // Contains all text from [msg], concatenated with '\n'.
  std::string text;

  std::optional<TEXTRENDER_RECT_ID> trr;
};
