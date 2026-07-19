///
/// Entity - Generic entity management
///

#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>

template <class T, std::size_t N, typename IndexType, typename ShouldDelete>
void Indsort(std::array<IndexType, N> &indices, IndexType &count,
             const std::array<T, N> &entities,
             ShouldDelete should_delete) noexcept {
  const auto mid =
      std::stable_partition(indices.begin(), indices.begin() + count,
                            [&entities, &should_delete](IndexType i) {
                              return !should_delete(entities[i]);
                            });
  count = static_cast<IndexType>(std::distance(indices.begin(), mid));
}
