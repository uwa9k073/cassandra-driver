#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace cassandra::io::detail {

// template <class Map>
// concept MapConcept = requires {
//     std::is_same_v<std::pair<typename Map::key_type, typename Map::mapped_type>, typename Map::value_type>;
// };

// template <MapConcept Map>
// struct BufferParser<Map> {
//     Map operator()(std::span<const std::byte> data, size_t& offset) {}
// };
}  // namespace cassandra::io::detail
