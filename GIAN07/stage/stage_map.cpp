///
/// StageMap - validated, decoded stage tile map
///

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "stage_map.h"

#include "util/byte_io.h"

namespace {

struct MapLayerHeader {
  uint32_t address;
  uint32_t scroll_wait;
  uint32_t length;
};

constexpr size_t kMapLayerLimit = 5;
constexpr size_t kMapHeaderSize = sizeof(uint32_t);
constexpr size_t kLayerHeaderSize = sizeof(uint32_t) * 3;
std::optional<MapLayerHeader> ReadLayerHeader(std::span<const uint8_t> data,
                                              size_t index) {
  const size_t offset = kMapHeaderSize + index * kLayerHeaderSize;
  if (offset > data.size() || data.size() - offset < kLayerHeaderSize) {
    return std::nullopt;
  }
  util::ByteReader reader(data.subspan(offset, kLayerHeaderSize));
  const auto address = reader.Read<uint32_t>();
  const auto scroll_wait = reader.Read<uint32_t>();
  const auto length = reader.Read<uint32_t>();
  if (!address || !scroll_wait || !length) {
    return std::nullopt;
  }
  return MapLayerHeader{
      .address = *address, .scroll_wait = *scroll_wait, .length = *length};
}

} // namespace

namespace stage {

std::optional<StageMap> StageMap::Parse(std::span<const uint8_t> bytes) {
  if (bytes.size() < kMapHeaderSize) {
    return std::nullopt;
  }

  util::ByteReader reader(bytes);
  const auto layer_count_value = reader.Read<uint32_t>();
  if (!layer_count_value) {
    return std::nullopt;
  }
  const uint32_t layer_count = *layer_count_value;
  const size_t data_start = kMapHeaderSize + layer_count * kLayerHeaderSize;
  if (layer_count == 0 || layer_count > kMapLayerLimit ||
      data_start > bytes.size()) {
    return std::nullopt;
  }

  StageMap map;
  map.layers_.reserve(layer_count);
  size_t previous_address = data_start;
  for (size_t layer_index = 0; layer_index < layer_count; ++layer_index) {
    const auto header = ReadLayerHeader(bytes, layer_index);
    if (!header || header->scroll_wait == 0 ||
        header->length < kVisibleMapRows + kMapTailRows ||
        header->address < data_start || header->address < previous_address ||
        header->address >= bytes.size()) {
      return std::nullopt;
    }

    Layer layer{.scroll_wait = header->scroll_wait};
    layer.rows.reserve(header->length);
    size_t offset = header->address;
    for (uint32_t row_index = 0; row_index < header->length; ++row_index) {
      Row row{};
      size_t column = 0;
      while (column < row.size()) {
        if (offset > bytes.size() || bytes.size() - offset < sizeof(uint16_t)) {
          return std::nullopt;
        }
        const auto value_read = util::ReadLittleAt<uint16_t>(bytes, offset);
        if (!value_read) {
          return std::nullopt;
        }
        const uint16_t value = *value_read;
        offset += sizeof(uint16_t);
        if (value != kEmptyMapTile) {
          if (value >= kMapTileCount) {
            return std::nullopt;
          }
          row[column++] = value;
          continue;
        }

        if (offset > bytes.size() || bytes.size() - offset < sizeof(uint16_t)) {
          return std::nullopt;
        }
        const auto run_read = util::ReadLittleAt<uint16_t>(bytes, offset);
        if (!run_read) {
          return std::nullopt;
        }
        const uint16_t run = *run_read;
        offset += sizeof(uint16_t);
        if (run == 0 || run > row.size() - column) {
          return std::nullopt;
        }
        std::fill_n(row.begin() + static_cast<ptrdiff_t>(column), run,
                    kEmptyMapTile);
        column += run;
      }
      layer.rows.push_back(row);
    }

    if (layer_index + 1 < layer_count) {
      const auto next = ReadLayerHeader(bytes, layer_index + 1);
      if (!next || offset > next->address) {
        return std::nullopt;
      }
    }
    previous_address = header->address;
    map.layers_.push_back(std::move(layer));
  }
  return map;
}

} // namespace stage
