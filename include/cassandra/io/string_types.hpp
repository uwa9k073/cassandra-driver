#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <string>
#include <cassandra/io/cassandra_types.hpp>

namespace cassandra::io::detail {
template <>
struct BufferParser<std::string> {
    std::string& value;
    explicit BufferParser(std::string& val) : value{val} {}

    void operator()(std::string_view buffer) {
        auto len = static_cast<SmallInt>(buffer[0]) << 8 | static_cast<SmallInt>(buffer[1]);
        value.assign(buffer.data() + 2, len);
    }
};
}  // namespace cassandra::io::detail
