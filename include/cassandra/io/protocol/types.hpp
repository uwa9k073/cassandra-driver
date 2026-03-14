#pragma once

#include <span>
#include <vector>

namespace cassandra::io::protocol {

using RawBuffer = std::vector<std::byte>;
using RawBufferView = std::span<const std::byte>;

struct Buffer {
    RawBuffer data;
    size_t offset = 0;

    RawBufferView view() const {
        return RawBufferView{data.data() + offset, data.size() - offset};
    }
};

}  // namespace cassandra::io::protocol
