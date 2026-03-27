#pragma once

#include <cassandra/io/cassandra_types.hpp>
#include <cstddef>
#include <span>
#include <vector>

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

enum class ErrorCode : Int {
    kServerError = 0x0000,
    kProtocolError = 0x000A,
    kAuthenticationError = 0x0100,

    kUnavailable = 0x1000,
    kOverloaded = 0x1001,
    kIsBootstrapping = 0x1002,
    kTruncateError = 0x1003,

    kWriteTimeout = 0x1100,
    kReadTimeout = 0x1200,

    kReadFailure = 0x1300,
    kFunctionFailure = 0x1400,
    kWriteFailure = 0x1500,

    kSyntaxError = 0x2000,
    kUnauthorized = 0x2100,
    kInvalid = 0x2200,
    kConfigError = 0x2300,
    kAlreadyExists = 0x2400,
    kUnprepared = 0x2500
};

}  // namespace cassandra::io::protocol
