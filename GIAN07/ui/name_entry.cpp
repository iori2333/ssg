/// Shared eight-character name entry control.

#include <algorithm>
#include <array>
#include <cstring>

#include "name_entry.h"

#include "audio/snd.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"

namespace {
constexpr int kBackspace = 0;
constexpr int kFinish = -1;
constexpr int kInvalid = -2;
} // namespace

void NameEntry::Begin(bool allow_cancel) {
  name_.fill('\0');
  cursor_x_ = 0;
  cursor_y_ = 0;
  key_repeat_ = 0;
  cursor_frame_ = 0;
  elapsed_ = 0;
  input_locked_ = Key_Data != 0U;
  allow_cancel_ = allow_cancel;
  awaiting_release_ = false;
  release_result_ = NameEntryResult::Confirmed;
}

int NameEntry::SelectedCharacter() const {
  if (cursor_y_ == 0) {
    return 'A' + (cursor_x_ % 26);
  }
  if (cursor_y_ == 1) {
    return 'a' + (cursor_x_ % 26);
  }
  if (cursor_x_ <= 9) {
    return '0' + cursor_x_;
  }
  constexpr std::array<char, 11> symbols = {'!', '?', '#', '\\', '<', '>',
                                            '=', ',', '+', '-',  ' '};
  if (cursor_x_ <= 20) {
    return symbols[cursor_x_ - 10];
  }
  if (cursor_x_ == 22) {
    return kBackspace;
  }
  if (cursor_x_ == 24) {
    return kFinish;
  }
  return kInvalid;
}

void NameEntry::Backspace() {
  const auto length = std::strlen(name_.data());
  if (length != 0) {
    name_[length - 1] = '\0';
  }
}

NameEntryResult NameEntry::Update(INPUT_BITS input) {
  if (awaiting_release_) {
    if (input == 0) {
      awaiting_release_ = false;
      return release_result_;
    }
    return NameEntryResult::Editing;
  }

  if (allow_cancel_ && !input_locked_ && input == KEY_ESC) {
    Snd_SEPlay(SfxId::Cancel);
    awaiting_release_ = true;
    release_result_ = NameEntryResult::Cancelled;
    return NameEntryResult::Editing;
  }

  bool finish = false;
  if (key_repeat_ == 0) {
    key_repeat_ = 8;
    switch (input) {
    case KEY_UP:
      cursor_y_ = (cursor_y_ + 2) % 3;
      Snd_SEPlay(SfxId::Select);
      break;
    case KEY_DOWN:
      cursor_y_ = (cursor_y_ + 1) % 3;
      Snd_SEPlay(SfxId::Select);
      break;
    case KEY_LEFT:
      cursor_x_ = cursor_y_ == 2 && cursor_x_ > 20 ? (cursor_x_ - 2) % 26
                                                   : (cursor_x_ + 25) % 26;
      Snd_SEPlay(SfxId::Select);
      break;
    case KEY_RIGHT:
      cursor_x_ = cursor_y_ == 2 && cursor_x_ >= 20 ? (cursor_x_ + 2) % 26
                                                    : (cursor_x_ + 1) % 26;
      Snd_SEPlay(SfxId::Select);
      break;
    case KEY_BOMB:
      Snd_SEPlay(SfxId::Cancel);
      Backspace();
      break;
    case KEY_TAMA:
    case KEY_RETURN: {
      if (input_locked_) {
        break;
      }
      Snd_SEPlay(SfxId::Select);
      const auto selected = SelectedCharacter();
      if (selected == kFinish || selected == kInvalid) {
        finish = true;
      } else if (selected == kBackspace) {
        Backspace();
      } else if (std::strlen(name_.data()) == name_.size() - 1) {
        cursor_x_ = 24;
        cursor_y_ = 2;
      } else {
        const auto length = std::strlen(name_.data());
        name_[length] = static_cast<char>(selected);
        name_[length + 1] = '\0';
      }
      break;
    }
    case 0:
      input_locked_ = false;
      break;
    }
    if (cursor_x_ > 20 && cursor_y_ == 2) {
      cursor_x_ &= ~1;
    }
  } else {
    key_repeat_--;
  }

  if (input == 0) {
    key_repeat_ = 0;
  }
  cursor_frame_ = (cursor_frame_ + 1) % 24;
  elapsed_++;

  if (finish) {
    if (name_[0] == '\0') {
      std::copy_n("Vivit!", 7, name_.data());
    }
    name_.back() = '\0';
    awaiting_release_ = true;
    release_result_ = NameEntryResult::Confirmed;
  }
  return NameEntryResult::Editing;
}

void NameEntry::Draw(int name_x, int name_y) const {
  GrpGeom->Lock();
  if (elapsed_ % 64 > 32) {
    GrpGeom->SetColor({4, 0, 0});
    const auto length = std::min(std::strlen(name_.data()), name_.size() - 2);
    const auto caret_x = name_x + static_cast<int>(length * 16);
    GrpGeom->DrawBox(caret_x, name_y, caret_x + 14, name_y + 16);
  }
  GrpGeom->Unlock();

  constexpr auto surface = SURFACE_ID::NAMEREG;
  GrpSurface_Blit({120, 0}, surface, {0, 0, 400, 64});
  GrpSurface_Blit({112, 420}, surface, {0, 432, 416, 480});

  PIXEL_LTRB cursor_src;
  if (cursor_x_ >= 20 && cursor_y_ == 2) {
    cursor_src = PIXEL_LTWH{432, 432 + ((cursor_frame_ >> 3) << 4), 32, 16};
  } else {
    cursor_src = PIXEL_LTWH{416, 432 + ((cursor_frame_ >> 3) << 4), 16, 16};
  }
  GrpSurface_Blit({112 + (cursor_x_ << 4), 420 + (cursor_y_ << 4)}, surface,
                  cursor_src);
}
