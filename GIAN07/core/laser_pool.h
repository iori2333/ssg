///
/// LaserPool - Fixed-capacity entity pool with index-based compaction
///

#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "core/entity.h"

template <typename T, std::size_t N>
struct LaserPool {
  std::array<T, N> data{};
  std::array<std::uint16_t, N> indices{};
  std::uint16_t count = 0;

  // Reset indices to 0..N-1 and count to 0.
  void Init() noexcept {
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(N); i++) {
      indices[i] = i;
    }
    count = 0;
  }

  // Allocate the next free slot. Returns nullptr if the pool is full.
  // The caller is responsible for initialising the returned object.
  [[nodiscard]] T *Alloc() noexcept {
    if (count == static_cast<std::uint16_t>(N)) {
      return nullptr;
    }
    return &data[indices[count++]];
  }

  // Access an active element by its logical position (0 ≤ i < count).
  T &Active(std::uint16_t i) noexcept { return data[indices[i]]; }
  [[nodiscard]] const T &Active(std::uint16_t i) const noexcept {
    return data[indices[i]];
  }

  // Map a logical active index to the raw slot index.
  [[nodiscard]] std::uint16_t RawIndex(std::uint16_t active_i) const noexcept {
    return indices[active_i];
  }

  // Reverse-map: given a pointer to an element inside data[],
  // return its raw slot index (data[T* - data.data()]).
  [[nodiscard]] std::uint16_t RawIndexOf(const T *p) const noexcept {
    return static_cast<std::uint16_t>(p - data.data());
  }

  // Compact the pool: move all elements for which is_dead returns true
  // past `count`, then shrink count to the first dead slot.
  template <typename Pred>
  void Compact(Pred is_dead) noexcept {
    Indsort(indices, count, data, std::move(is_dead));
  }

  [[nodiscard]] bool IsFull() const noexcept {
    return count == static_cast<std::uint16_t>(N);
  }
  [[nodiscard]] bool IsEmpty() const noexcept { return count == 0; }
  [[nodiscard]] std::uint16_t Size() const noexcept { return count; }
};
