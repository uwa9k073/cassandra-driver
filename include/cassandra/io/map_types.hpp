#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace cassandra::io::detail {
template <>
struct BufferParser<std::unordered_map<std::string, std::vector<std::string>>> {
    std::unordered_map<std::string, std::vector<std::string>> operator()(BufferReader buffer_reader) {
        return buffer_reader.ReadStringMultiMap();
    }
};
}  // namespace cassandra::io::detail
