///
/// ObjectPool - Fixed-capacity entity pool with index-based compaction
///

#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

#include "core/entity.h"

template <typename T, std::size_t N>
struct ObjectPool {
  // ── Forward iterator ─────────────────────────────────────────
  template <bool IsConst>
  struct Iterator {
    using iterator_concept = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;

    using pointer = std::conditional_t<IsConst, const T *, T *>;
    using reference = std::conditional_t<IsConst, const T &, T &>;

    using DataPtr = std::conditional_t<IsConst, const T *, T *>;

    constexpr Iterator(DataPtr data, const std::size_t *indices,
                       std::size_t pos) noexcept
        : data_(data), indices_(indices), pos_(pos) {}

    constexpr reference operator*() const noexcept {
      return data_[indices_[pos_]];
    }
    constexpr pointer operator->() const noexcept {
      return &data_[indices_[pos_]];
    }

    constexpr Iterator &operator++() noexcept {
      ++pos_;
      return *this;
    }
    constexpr Iterator operator++(int) noexcept {
      auto tmp = *this;
      ++pos_;
      return tmp;
    }

    constexpr bool operator==(const Iterator &other) const noexcept {
      return pos_ == other.pos_;
    }

  private:
    DataPtr data_{};
    const std::size_t *indices_{};
    std::size_t pos_{};
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  // ── Iteration ────────────────────────────────────────────────
  constexpr iterator begin() noexcept {
    return {data_.data(), indices_.data(), 0};
  }
  constexpr iterator end() noexcept {
    return {data_.data(), indices_.data(), count_};
  }
  constexpr const_iterator begin() const noexcept {
    return {data_.data(), indices_.data(), 0};
  }
  constexpr const_iterator end() const noexcept {
    return {data_.data(), indices_.data(), count_};
  }

  // ── Operations ───────────────────────────────────────────────
  void Init() noexcept {
    for (std::size_t i = 0; i < N; i++) {
      indices_[i] = i;
    }
    count_ = 0;
  }

  [[nodiscard]] T *Alloc() noexcept {
    if (count_ == N) {
      return nullptr;
    }
    return &data_[indices_[count_++]];
  }

  T &Active(std::size_t i) noexcept { return data_[indices_[i]]; }
  [[nodiscard]] const T &Active(std::size_t i) const noexcept {
    return data_[indices_[i]];
  }

  [[nodiscard]] std::size_t RawIndex(std::size_t active_i) const noexcept {
    return indices_[active_i];
  }

  [[nodiscard]] std::size_t RawIndexOf(const T *p) const noexcept {
    return static_cast<std::size_t>(p - data_.data());
  }

  template <typename Pred> void Compact(Pred is_dead) noexcept {
    Indsort(indices_, count_, data_, std::move(is_dead));
  }

  [[nodiscard]] bool IsFull() const noexcept { return count_ == N; }
  [[nodiscard]] bool IsEmpty() const noexcept { return count_ == 0; }
  [[nodiscard]] std::size_t Size() const noexcept { return count_; }

private:
  std::array<T, N> data_{};
  std::array<std::size_t, N> indices_{};
  std::size_t count_ = 0;
};
