///
/// StageMap - validated, decoded stage tile map
///
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace stage {

inline constexpr size_t kMapWidth = 24;
inline constexpr size_t kVisibleMapRows = 31;
inline constexpr size_t kMapTailRows = 80;
inline constexpr uint16_t kEmptyMapTile = 0xffff;
inline constexpr uint16_t kMapTileCount = 1200;

class StageMap {
public:
  using Row = std::array<uint16_t, kMapWidth>;

  struct Layer {
    std::vector<Row> rows;
    uint32_t scroll_wait = 0;
  };

  [[nodiscard]] static std::optional<StageMap>
  Parse(std::span<const uint8_t> bytes);

  [[nodiscard]] const std::vector<Layer> &Layers() const { return layers_; }

private:
  std::vector<Layer> layers_;
};

} // namespace stage
