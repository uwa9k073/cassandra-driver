#pragma once

#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/traits.hpp>
#include <cassandra/io/value.hpp>
#include <cassert>
#include <chrono>
#include <cstring>
#include <userver/utils/strong_typedef.hpp>

namespace cassandra::io {
using Timestamp =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

namespace detail {

struct TimestampParser : BufferParserBase<Timestamp> {
    using BaseType = BufferParserBase<Timestamp>;
    using BaseType::BaseType;

    void operator()(const Bytes& buffer) {
        const auto& payload = std::get<1>(buffer.payload);

        if (payload.size() != sizeof(BigInt)) {
            throw exceptions::Error("Invalid Buffer Size");
        }

        const auto& time = ReadBuffer<BigInt>(buffer);

        this->value = Timestamp{std::chrono::milliseconds{time}};
    }
};

struct TimestampFormatter : BufferFormatterBase<Timestamp>,
                            ValueFormattingMixin<TimestampFormatter> {
    using BaseType = BufferFormatterBase<Timestamp>;
    using BaseType::BaseType;
    using Mixin::operator();

    void operator()(Bytes& buffer) {
        BigInt time = std::chrono::duration_cast<std::chrono::milliseconds>(
                          this->value.time_since_epoch()
        )
                          .count();

        WriteBuffer(buffer, time);
    }
};
}  // namespace detail

template <>
struct BufferParser<Timestamp> : detail::TimestampParser {
    using detail::TimestampParser::TimestampParser;
};

template <>
struct BufferFormatter<Timestamp> : detail::TimestampFormatter {
    using detail::TimestampFormatter::TimestampFormatter;
};
}  // namespace cassandra::io
