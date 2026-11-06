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
#include <userver/utils/time_of_day.hpp>

namespace cassandra::io {

using Time = userver::utils::datetime::TimeOfDay<std::chrono::nanoseconds>;

namespace detail {

struct TimeOfDayParser : BufferParserBase<Time> {
    using BaseType = BufferParserBase<Time>;
    using BaseType::BaseType;

    void operator()(const Bytes& buffer) {
        protocol::RawBufferView payload = std::get<1>(buffer.payload);

        if (payload.size() != sizeof(BigInt)) {
            throw exceptions::Error("Invalid time size");
        }

        auto val = ReadBuffer<UBigInt>(buffer);

        this->value = Time{std::chrono::nanoseconds{val}};
    }
};

struct TimeOfDayFormatter : BufferFormatterBase<Time>,
                            ValueFormattingMixin<TimeOfDayFormatter> {
    using BaseType = BufferFormatterBase<Time>;
    using BaseType::BaseType;
    using Mixin::operator();

    void operator()(Bytes& buffer) {
        UBigInt time = this->value.SinceMidnight().count();

        WriteBuffer(buffer, time);
    }
};
}  // namespace detail

template <>
struct BufferParser<Time> : detail::TimeOfDayParser {
    using detail::TimeOfDayParser::TimeOfDayParser;
};

template <>
struct BufferFormatter<Time> : detail::TimeOfDayFormatter {
    using detail::TimeOfDayFormatter::TimeOfDayFormatter;
};
}  // namespace cassandra::io
