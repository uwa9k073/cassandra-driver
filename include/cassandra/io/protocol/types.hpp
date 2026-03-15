#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include "cassandra/io/cassandra_types.hpp"

namespace cassandra::io::protocol {

using RawBuffer = std::vector<std::byte>;
using RawBufferView = std::span<const std::byte>;

using BytesBuffer = std::vector<Bytes>;
using BytesBufferView = std::span<const Bytes>;

struct Buffer {
    RawBuffer data;
    size_t offset = 0;

    RawBufferView View() const {
        return RawBufferView{data.data() + offset, data.size() - offset};
    }

    RawBufferView View(size_t size) const {
        return RawBufferView{data.data() + offset, size};
    }
};

struct BufferView {
    RawBufferView data;
    size_t offset = 0;

    RawBufferView SubView(size_t size) const { return data.subspan(offset, size); }
};

}  // namespace cassandra::io::protocol
