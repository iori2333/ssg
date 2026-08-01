/// Shared eight-character name entry control.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "sys/input.h"

namespace audio { class AudioSystem; }

enum class NameEntryResult : uint8_t { Editing, Confirmed, Cancelled };
inline constexpr std::size_t kNameEntryLength = 8;

class NameEntry {
public:
  explicit NameEntry(audio::AudioSystem &audio) : audio_(audio) {}

  void Begin(bool allow_cancel, InputBits initial_input);
  [[nodiscard]] NameEntryResult Update(InputBits input);
  void Draw(int name_x, int name_y) const;

  [[nodiscard]] std::string_view Name() const { return name_.data(); }

private:
  audio::AudioSystem &audio_;

  [[nodiscard]] int SelectedCharacter() const;
  void Backspace();

  std::array<char, kNameEntryLength + 1> name_{};
  int cursor_x_ = 0;
  int cursor_y_ = 0;
  int8_t key_repeat_ = 0;
  uint8_t cursor_frame_ = 0;
  uint8_t elapsed_ = 0;
  bool input_locked_ = false;
  bool allow_cancel_ = false;
  bool awaiting_release_ = false;
  NameEntryResult release_result_ = NameEntryResult::Confirmed;
};
