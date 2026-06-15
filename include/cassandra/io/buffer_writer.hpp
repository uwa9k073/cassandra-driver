#pragma once

#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/floating_point_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/optional_values.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/string_types.hpp>

namespace cassandra::io {

template <class Buffer>
class BufferWriter {
public:
    using BufferType = Buffer;
    // CTAD: Template argument 'Buffer' is deduced from the constructor
    // argument
    explicit BufferWriter(BufferType& buffer) noexcept : buffer_(buffer) {}

    template <class T>
    void Write(const T& value) {
        // Overload resolution happens here based on the concrete 'Buffer' type
        WriteBuffer<T>(buffer_, value);
    }

private:
    BufferType& buffer_;
};

}  // namespace cassandra::io
