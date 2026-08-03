///
/// Rectangle packing for text rendering (adapted from rectpack2D)
///

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

#include "text_renderer.h"

#include "sys/log.h"

namespace {

struct CreatedSplits {
  int count = 0;
  std::array<Rect, 2> spaces;

  static auto Failed() {
    CreatedSplits result;
    result.count = -1;
    return result;
  }

  template <typename... Args>
  CreatedSplits(Args &&...args) noexcept
      : count(sizeof...(Args)), spaces({std::forward<Args>(args)...}) {}

  explicit operator bool() const { return count != -1; }
};

CreatedSplits InsertAndSplit(const PixelPoint &nw, const Rect &sp) {
  const auto free_w = sp.Width() - nw.x;
  const auto free_h = sp.Height() - nw.y;

  if (free_w < 0 || free_h < 0) {
    // Image is bigger than the candidate empty space.
    // We'll need to look further.
    return CreatedSplits::Failed();
  }

  if ((free_w == 0) && (free_h == 0)) {
    // If the image dimensions equal the dimensions of the candidate empty
    // space (image fits exactly), we will just delete the space and create
    // no splits.
    return {};
  }

  // If the image fits into the candidate empty space, but exactly one of the
  // image dimensions equals the respective dimension of the candidate empty
  // space (e.g. image = 20x40, candidate space = 30x40) we delete the space
  // and create a single split. In this case a 10x40 space.
  if ((free_w > 0) && (free_h == 0)) {
    auto r = sp;
    r.left += nw.x;
    return {r};
  }
  if ((free_w == 0) && (free_h > 0)) {
    auto r = sp;
    r.top += nw.y;
    return {r};
  }

  // Every other option has been exhausted, so at this point the image must
  // be *strictly* smaller than the empty space, that is, it is smaller in
  // both width and height.
  // Thus, free_w and free_h must be positive.

  // Decide which way to split.
  //
  // Instead of having two normally-sized spaces, it is better - though I
  // have no proof of that - to have a one tiny space and a one huge space.
  // This creates better opportunity for insertion of future rectangles.
  //
  // This is why, if we had more of width remaining than we had of height,
  // we split along the vertical axis, and if we had more of height remaining
  // than we had of width, we split along the horizontal axis.
  Rect bigger_split;
  Rect lesser_split;
  if (free_w > free_h) {
    bigger_split = Rect::FromLtwh(sp.left + nw.x, sp.top, free_w, sp.Height());
    lesser_split = Rect::FromLtwh(sp.left, sp.top + nw.y, nw.x, free_h);
  } else {
    bigger_split = Rect::FromLtwh(sp.left, sp.top + nw.y, sp.Width(), free_h);
    lesser_split = Rect::FromLtwh(sp.left + nw.x, sp.top, free_w, nw.y);
  }
  return {bigger_split, lesser_split};
}

} // namespace

std::optional<Rect> TextRender::Insert(PixelPoint subrect_size) {
  if (!subrect_size) {
    logging::Critical(logging::Channel::Graphics,
                      "Cannot pack a text rectangle with size {}x{}",
                      subrect_size.x, subrect_size.y);
    return std::nullopt;
  }

  while (true) {
    Rect *closest = nullptr;

    for (int i = static_cast<int>(spaces_.size()) - 1; i >= 0; --i) {
      const Rect candidate = spaces_[i];

      if ((closest == nullptr) || ((candidate.Width() * candidate.Height()) <
                                   (closest->Width() * closest->Height()))) {
        closest = &spaces_[i];
      }

      const auto splits = InsertAndSplit(subrect_size, candidate);
      if (splits) {
        spaces_[i] = spaces_.back();
        spaces_.pop_back();

        for (int s = 0; s < splits.count; ++s) {
          AddSpace(splits.spaces[s]);
        }

        const Rect ret = Rect::FromLtwh(candidate.left, candidate.top,
                                        subrect_size.x, subrect_size.y);
        bounds_.x = std::max(bounds_.x, ret.right);
        bounds_.y = std::max(bounds_.y, ret.bottom);
        return ret;
      }
    }

    // Expand the closest space in-place, but only if this would add fewer
    // pixels than starting a new row or column. The bounds are resized
    // accordingly during the actual insertion.
    if ((closest != nullptr) &&
        ((subrect_size.x - closest->Width()) < subrect_size.y) &&
        ((subrect_size.y - closest->Height()) < subrect_size.x)) {
      closest->right = closest->left + subrect_size.x;
      closest->bottom = closest->top + subrect_size.y;
    } else {
      constexpr auto coord_max = std::numeric_limits<int>::max();

      if (bounds_.x <= bounds_.y) {
        if (subrect_size.x > (coord_max - bounds_.x)) {
          logging::Critical(logging::Channel::Graphics,
                            "Text rectangle packing exceeded coordinate range");
          return std::nullopt;
        }
        AddSpace(Rect::FromLtwh(bounds_.x, 0, subrect_size.x,
                                std::max(bounds_.y, subrect_size.y)));
      } else {
        if (subrect_size.y > (coord_max - bounds_.y)) {
          logging::Critical(logging::Channel::Graphics,
                            "Text rectangle packing exceeded coordinate range");
          return std::nullopt;
        }
        AddSpace(Rect::FromLtwh(
            0, bounds_.y, std::max(bounds_.x, subrect_size.x), subrect_size.y));
      }
    }
  }
}

TextRender::Entry *TextRender::Find(TextRenderRectId id) {
  if (id.generation != generation_ || id.index >= entries_.size()) {
    logging::Critical(logging::Channel::Graphics,
                      "Invalid text cache handle: {}:{}", id.generation,
                      id.index);
    return nullptr;
  }
  return &entries_[id.index];
}

const TextRender::Entry *TextRender::Find(TextRenderRectId id) const {
  return const_cast<TextRender *>(this)->Find(id);
}

std::optional<Rect>
TextRender::Subrect(TextRenderRectId id,
                    std::optional<Rect> maybe_subrect) const {
  const auto *entry = Find(id);
  if (entry == nullptr) {
    return std::nullopt;
  }
  auto ret = entry->rect;
  if (maybe_subrect) {
    const auto &subrect = maybe_subrect.value();
    const auto available_width = ret.Width();
    const auto available_height = ret.Height();
    const auto left = std::clamp(subrect.left, 0, available_width);
    const auto top = std::clamp(subrect.top, 0, available_height);
    ret.left += left;
    ret.top += top;
    ret.right =
        ret.left + std::clamp(subrect.Width(), 0, available_width - left);
    ret.bottom =
        ret.top + std::clamp(subrect.Height(), 0, available_height - top);
  }
  return ret;
}

TextRenderRectId TextRender::Register(PixelPoint size) {
  const auto rect = Insert(size);
  if (!rect) {
    return {};
  }
  entries_.push_back({.rect = *rect});
  return {.index = entries_.size() - 1, .generation = generation_};
}

void TextRender::Wipe() {
  for (auto &entry : entries_) {
    entry.cache_key = std::nullopt;
  }
}

void TextRender::Clear() {
  bounds_ = {};
  spaces_.clear();
  entries_.clear();
  ++generation_;
  if (generation_ == 0) {
    generation_ = 1;
  }
}

bool TextRender::Blit(PixelPoint dst, TextRenderRectId rect_id,
                      std::optional<Rect> subrect) {
  const auto rect = Subrect(rect_id, subrect);
  if (!rect) {
    return false;
  }
  return GraphicsSurfaceBlit(dst, SurfaceId::Text, *rect);
}
