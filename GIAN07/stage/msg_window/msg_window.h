///
/// MsgWindow - Message window processing
///

#pragma once

#include <optional>
#include <string>

#include "gfx/text.h"
#include "stage/menu/window_sys.h"

// Message window flags
enum class MsgWindowFlags : uint8_t {
  NONE = 0x0,
  WITH_FACE = 0x1, // Pads all text to leave room for a face portrait.
  CENTER = 0x2,    // Horizontally centers all text.
  HAS_BITFLAG_OPERATORS = 3,
};

// Message window management class
// Originally a [MSG_WINDOW] struct + file-static global [MsgWindow], but
// was made a class to encapsulate state. The [MWin*] free functions remain
// as thin wrappers around this global instance.
class MsgWindow {
public:
  void Init(const WINDOW_LTRB &rc, MsgWindowFlags flags = MsgWindowFlags::NONE);
  void Open();                    // Open the message window
  void Close();                   // Close the message window
  void ForceClose();              // Force close the message window
  void Tick();                    // Run message window logic
  void Draw();                    // Draw the message window
  void Msg(std::string_view str); // Send a message string
  void Face(uint8_t faceID);      // Set the face portrait
  void Cmd(uint8_t cmd);          // Send a command
  void Help(MenuController *ws);  // Send help text to the message window

private:
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

  std::string_view msg[MSG_HEIGHT]{}; // Pointers to displayed messages

  // Contains all text from [msg], concatenated with '\n'.
  std::string text;

  std::optional<TEXTRENDER_RECT_ID> trr;
};

