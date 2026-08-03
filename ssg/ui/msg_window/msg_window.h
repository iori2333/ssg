///
/// MsgWindow - Message window processing
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "gfx/text/text.h"
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
  void Init(const Rect &rc, MsgWindowFlags flags = MsgWindowFlags::None);
  void Open();       // Open the message window
  void Close();      // Close the message window
  void ForceClose(); // Force close the message window
  void Tick();       // Run message window logic
  void Draw();       // Draw the message window
  void AppendMessage(std::string_view message);
  void SetFace(std::size_t face_id);
  void SetFont(FontId font);
  void NewPage();
  void ShowHelp(std::string_view help);

private:
  enum class State : uint8_t { Dead, Open, Close, Free };
  enum class FaceState : uint8_t { None, Open, Close, Next, Wait };

  static constexpr auto kMessageLines = 5;
  static constexpr int kFaceFrameStep = 16;
  static constexpr int kFaceFrameCount = 256;

  void MsgBlank(); // Clear strings and reset to first line

  Rect max_size{}; // Final window size
  Rect now_size{}; // Current window size
  PixelPoint text_topleft{};

  MsgWindowFlags flags{};
  FontId font_id{}; // Font to use
  int font_dy{};    // Font Y increment
  State state = State::Dead;
  std::size_t max_line{}; // Max displayable lines
  std::size_t line{};     // Next line index

  std::size_t face_id{};   // Current face ID
  std::size_t next_face{}; // Next face ID to show
  FaceState face_state = FaceState::None;
  int face_time{}; // Face display counter

  std::array<std::string_view, kMessageLines> msg{}; // Displayed messages

  // Contains all text from [msg], concatenated with '\n'.
  std::string text;

  std::optional<TextRenderRectId> trr;
};
