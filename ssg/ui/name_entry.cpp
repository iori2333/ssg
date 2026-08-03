/// Shared eight-character name entry control.

#include <algorithm>
#include <array>
#include <cstring>

#include "name_entry.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "gfx/core/constants.h"
#include "gfx/graphics.h"
#include "gfx/render/geometry.h"
#include "sys/input.h"

namespace {
constexpr int kBackspace = 0;
constexpr int kFinish = -1;
constexpr int kInvalid = -2;
} // namespace

void NameEntry::Begin(bool allow_cancel, InputBits initial_input) {
  name_.fill('\0');
  cursor_x_ = 0;
  cursor_y_ = 0;
  key_repeat_ = 0;
  cursor_frame_ = 0;
  elapsed_ = 0;
  input_locked_ = initial_input != 0U;
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

NameEntryResult NameEntry::Update(InputBits input) {
  if (awaiting_release_) {
    if (input == 0) {
      awaiting_release_ = false;
      return release_result_;
    }
    return NameEntryResult::Editing;
  }

  if (allow_cancel_ && !input_locked_ && input == KeyEscape) {
    audio_.PlaySfx(SfxId::Cancel);
    awaiting_release_ = true;
    release_result_ = NameEntryResult::Cancelled;
    return NameEntryResult::Editing;
  }

  bool finish = false;
  if (key_repeat_ == 0) {
    key_repeat_ = 8;
    switch (input) {
    case KeyUp:
      cursor_y_ = (cursor_y_ + 2) % 3;
      audio_.PlaySfx(SfxId::Select);
      break;
    case KeyDown:
      cursor_y_ = (cursor_y_ + 1) % 3;
      audio_.PlaySfx(SfxId::Select);
      break;
    case KeyLeft:
      cursor_x_ = cursor_y_ == 2 && cursor_x_ > 20 ? (cursor_x_ - 2) % 26
                                                   : (cursor_x_ + 25) % 26;
      audio_.PlaySfx(SfxId::Select);
      break;
    case KeyRight:
      cursor_x_ = cursor_y_ == 2 && cursor_x_ >= 20 ? (cursor_x_ + 2) % 26
                                                    : (cursor_x_ + 1) % 26;
      audio_.PlaySfx(SfxId::Select);
      break;
    case KeyBomb:
      audio_.PlaySfx(SfxId::Cancel);
      Backspace();
      break;
    case KeyTama:
    case KeyReturn: {
      if (input_locked_) {
        break;
      }
      audio_.PlaySfx(SfxId::Select);
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
    default:
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
  if (elapsed_ % 64 > 32) {
    geometry::SetColor({4, 0, 0});
    const auto length = std::min(std::strlen(name_.data()), name_.size() - 2);
    const auto caret_x = name_x + static_cast<int>(length * 16);
    geometry::DrawBox(caret_x, name_y, caret_x + 14, name_y + 16);
  }

  constexpr auto surface = SurfaceId::NameRegistration;
  GraphicsSurfaceBlit({120, 0}, surface, {0, 0, 400, 64});
  GraphicsSurfaceBlit({112, 420}, surface, {0, 432, 416, 480});

  Rect cursor_src;
  if (cursor_x_ >= 20 && cursor_y_ == 2) {
    cursor_src = Rect::FromLtwh(432, 432 + ((cursor_frame_ >> 3) << 4), 32, 16);
  } else {
    cursor_src = Rect::FromLtwh(416, 432 + ((cursor_frame_ >> 3) << 4), 16, 16);
  }
  GraphicsSurfaceBlit({112 + (cursor_x_ << 4), 420 + (cursor_y_ << 4)}, surface,
                      cursor_src);
}
