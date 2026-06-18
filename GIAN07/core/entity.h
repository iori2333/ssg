///
/// Entity - Generic entity management
///

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

template <class T, std::size_t N, typename ShouldDelete>
void Indsort(std::array<std::uint16_t, N> &indices, std::uint16_t &count,
             const std::array<T, N> &entities,
             ShouldDelete should_delete) noexcept {
  // stable_partition: keep alive entities first, preserve relative order
  const auto mid =
      std::stable_partition(indices.begin(), indices.begin() + count,
                            [&entities, &should_delete](std::uint16_t i) {
                              return !should_delete(entities[i]);
                            });
  count = static_cast<std::uint16_t>(std::distance(indices.begin(), mid));
}
