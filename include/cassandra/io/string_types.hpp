#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <string>

namespace cassandra::io::detail {
template <>
struct BufferParser<std::string> {
    std::string operator()(BufferReader buffer_reader) { return buffer_reader.ReadString(); }
};
}  // namespace cassandra::io::detail
