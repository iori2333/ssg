///
/// Rectangle packing for text rendering (adapted from rectpack2D)
///

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

#include "constants.h"
#include "coords.h"
#include "graphics_backend.h"
#include "text.h"
#include "text_packed.h"

#include "sys/log.h"

namespace {

struct created_splits {
  int count = 0;
  std::array<PixelLtwh, 2> spaces;

  static auto failed() {
    created_splits result;
    result.count = -1;
    return result;
  }

  static auto none() { return created_splits(); }

  template <typename... Args>
  created_splits(Args &&...args) noexcept
      : count(sizeof...(Args)), spaces({std::forward<Args>(args)...}) {}

  explicit operator bool() const { return count != -1; }
};

created_splits insert_and_split(const PixelSize &nw, const PixelLtwh &sp) {
  const auto free_w = (sp.w - nw.w);
  const auto free_h = (sp.h - nw.h);

  if (free_w < 0 || free_h < 0) {
    // Image is bigger than the candidate empty space.
    // We'll need to look further.
    return created_splits::failed();
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
    r.left += nw.w;
    r.w -= nw.w;
    return {r};
  }
  if ((free_w == 0) && (free_h > 0)) {
    auto r = sp;
    r.top += nw.h;
    r.h -= nw.h;
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
  PixelLtwh bigger_split;
  PixelLtwh lesser_split;
  if (free_w > free_h) {
    bigger_split = {(sp.left + nw.w), sp.top, free_w, sp.h};
    lesser_split = {sp.left, (sp.top + nw.h), nw.w, free_h};
  } else {
    bigger_split = {sp.left, (sp.top + nw.h), sp.w, free_h};
    lesser_split = {(sp.left + nw.w), sp.top, free_w, nw.h};
  }
  return {bigger_split, lesser_split};
}

} // namespace

PixelLtwh TextRenderPacked::Insert(const PixelSize &subrect_size) {
  if (!subrect_size) {
    logging::Critical(logging::Channel::Graphics,
                      "Cannot pack a text rectangle with size {}x{}",
                      subrect_size.w, subrect_size.h);
    return {};
  }

  while (true) {
    PixelLtwh *closest = nullptr;

    for (int i = static_cast<int>(spaces.size()) - 1; i >= 0; --i) {
      const PixelLtwh candidate = spaces[i];

      if ((closest == nullptr) ||
          ((candidate.w * candidate.h) < (closest->w * closest->h))) {
        closest = &spaces[i];
      }

      const auto splits = insert_and_split(subrect_size, candidate);
      if (splits) {
        spaces[i] = spaces.back();
        spaces.pop_back();

        for (int s = 0; s < splits.count; ++s) {
          SpaceAdd(splits.spaces[s]);
        }

        const PixelLtwh ret = {candidate.left, candidate.top, subrect_size.w,
                               subrect_size.h};
        bounds.w = std::max(bounds.w, (ret.left + ret.w));
        bounds.h = std::max(bounds.h, (ret.top + ret.h));
        return ret;
      }
    }

    // Expand the closest space in-place, but only if this would add fewer
    // pixels than starting a new row or column. The bounds are resized
    // accordingly during the actual insertion.
    if ((closest != nullptr) &&
        ((subrect_size.w - closest->w) < subrect_size.h) &&
        ((subrect_size.h - closest->h) < subrect_size.w)) {
      closest->w = subrect_size.w;
      closest->h = subrect_size.h;
    } else {
      constexpr auto coord_max = std::numeric_limits<PixelCoord>::max();

      if (bounds.w <= bounds.h) {
        if (subrect_size.w > (coord_max - bounds.w)) {
          logging::Critical(logging::Channel::Graphics,
                            "Text rectangle packing exceeded coordinate range");
          return {};
        }
        SpaceAdd(PixelLtwh{bounds.w, 0, subrect_size.w,
                           std::max(bounds.h, subrect_size.h)});
      } else {
        if (subrect_size.h > (coord_max - bounds.h)) {
          logging::Critical(logging::Channel::Graphics,
                            "Text rectangle packing exceeded coordinate range");
          return {};
        }
        SpaceAdd(PixelLtwh{0, bounds.h, std::max(bounds.w, subrect_size.w),
                           subrect_size.h});
      }
    }
  }
}

PixelLtwh TextRenderPacked::Subrect(TextRenderRectId rect_id,
                                    std::optional<PixelLtwh> maybe_subrect) {
  if (rect_id >= rects.size()) {
    logging::Critical(logging::Channel::Graphics,
                      "Invalid text rectangle ID: {}", rect_id);
    return {};
  }
  auto ret = rects[rect_id].rect;
  if (maybe_subrect) {
    const auto &subrect = maybe_subrect.value();
    ret.left += subrect.left;
    ret.top += subrect.top;
    ret.w = (std::min)(subrect.w, ret.w);
    ret.h = (std::min)(subrect.h, ret.h);
  }
  return ret;
}

TextRenderRectId TextRenderPacked::Register(const PixelSize &size) {
  rects.emplace_back(Insert(size));
  return static_cast<TextRenderRectId>(rects.size() - 1);
}

bool TextRenderPacked::Wipe() {
  for (auto &rect : rects) {
    rect.contents = std::nullopt;
  }
  return true;
}

void TextRenderPacked::Clear() {
  bounds = {};
  spaces.clear();
  rects.clear();
}

bool TextRenderPacked::Blit(WindowPoint dst, TextRenderRectId rect_id,
                            std::optional<PixelLtwh> subrect) {
  if (rect_id >= rects.size()) {
    logging::Critical(logging::Channel::Graphics,
                      "Invalid text rectangle ID: {}", rect_id);
    return false;
  }
  const PixelLtrb rect = Subrect(rect_id, subrect);
  return GraphicsSurfaceBlit(dst, SurfaceId::Text, rect);
}
