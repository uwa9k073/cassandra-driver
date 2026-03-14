#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/floating_point_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/string_types.hpp>
#include <span>
#include "cassandra/io/protocol/types.hpp"
namespace cassandra::io {
class BufferReader {
public:
    explicit BufferReader(std::span<const std::byte> data) noexcept
        : data_(data), offset_(0) {}

    // ===== Core API =====
    [[nodiscard]] size_t Offset() const noexcept { return offset_; }
    [[nodiscard]] size_t Remaining() const noexcept {
        return data_.size() - offset_;
    }
    [[nodiscard]] bool Empty() const noexcept { return offset_ >= data_.size(); }

    template <class T>
    [[nodiscard]] T Read() {
        return detail::Read<T>(this->data_, this->offset_);
    }

    protocol::RawBufferView GetSubBuffer(size_t size) {
        return data_.subspan(offset_, size);
    }

private:
    std::span<const std::byte> data_;
    size_t offset_ = 0;
};
}  // namespace cassandra::io
