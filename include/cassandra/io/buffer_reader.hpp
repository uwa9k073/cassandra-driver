#pragma once

#include <fmt/core.h>
#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/floating_point_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/string_types.hpp>
#include <cassert>

namespace cassandra::io {
template <class Buffer>
class BufferReader;

template <>
class BufferReader<protocol::BufferView> {
public:
    explicit BufferReader(protocol::RawBufferView buffer) noexcept
        : _buffer(protocol::BufferView{buffer, 0}) {}

    // ===== Core API =====
    [[nodiscard]] size_t Offset() const noexcept { return _buffer.offset; }
    [[nodiscard]] size_t Remaining() const noexcept {
        return _buffer.data.size() - _buffer.offset;
    }
    [[nodiscard]] bool Empty() const noexcept { return _buffer.data.empty(); }

    template <class T>
    [[nodiscard]] T Read() {
        return ReadBuffer<T>(_buffer.data, _buffer.offset);
    }

    protocol::RawBufferView GetSubBuffer(size_t size) {
        return _buffer.SubView(size);
    }

private:
    protocol::BufferView _buffer;
};

template <>
class BufferReader<Bytes> {
public:
    explicit BufferReader(const Bytes& buffer) noexcept : _buffer(buffer) {}

    template <class T>
    [[nodiscard]] T Read() {
        return ReadBuffer<T>(_buffer);
    }

private:
    const Bytes& _buffer;
};
}  // namespace cassandra::io
