/// Shared eight-character name entry control.

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "sys/input.h"

enum class NameEntryResult : uint8_t { Editing, Confirmed, Cancelled };

class NameEntry {
public:
  void Begin(bool allow_cancel);
  [[nodiscard]] NameEntryResult Update(INPUT_BITS input);
  void Draw(int name_x, int name_y) const;

  [[nodiscard]] std::string_view Name() const { return name_.data(); }

private:
  [[nodiscard]] int SelectedCharacter() const;
  void Backspace();

  std::array<char, 9> name_{};
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
