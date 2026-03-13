#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/floating_point_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/string_types.hpp>
#include <cstddef>
#include <vector>

namespace cassandra::io {
class BufferWriter {
public:
    explicit BufferWriter(std::vector<std::byte>& buffer) noexcept
        : buffer_(buffer) {}

    template <class T>
    void Write(const T& value) {
        detail::Write<T>(buffer_, value);
    }

    void AddBuffer(protocol::RawBufferView value) {
        auto offset = buffer_.size();
        buffer_.resize(offset + value.size());
        std::copy(value.begin(), value.end(), buffer_.begin() + offset);
    }

private:
    std::vector<std::byte>& buffer_;
};

}  // namespace cassandra::io
