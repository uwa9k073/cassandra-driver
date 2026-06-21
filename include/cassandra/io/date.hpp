#pragma once

#include <bits/chrono.h>
#include <date/date.h>
#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/traits.hpp>
#include <cassandra/io/value.hpp>
#include <chrono>
namespace cassandra::io {

namespace detail {

struct DateParser : BufferParserBase<Date> {
    using BaseType = BufferParserBase<Date>;
    using BaseType::BaseType;

    void operator()(const Bytes& buffer) {
        UInt raw_value = ReadBuffer<UInt>(buffer);

        long long days_since_epoch = static_cast<long long>(raw_value) - (1LL << 31);

        this->value = userver::utils::datetime::Date{
            std::chrono::sys_days{std::chrono::days{days_since_epoch}}
        };
    }
};

struct DateFormatter : BufferFormatterBase<Date>,
                       ValueFormattingMixin<DateFormatter> {
    using BaseType = BufferFormatterBase<Date>;
    using Mixin = ValueFormattingMixin<DateFormatter>;

    using BaseType::BaseType;
    using Mixin::operator();

    void operator()(Bytes& buffer) {
        auto days_since_epoch = this->value.GetSysDays().time_since_epoch().count();

        Bytes::UnderlyingType payload;

        auto time = static_cast<UInt>(days_since_epoch + (1ULL << 31));

        WriteBuffer<UInt>(payload, time);

        buffer.payload = std::move(payload);
    }
};
}  // namespace detail

template <>
struct BufferFormatter<Date> : detail::DateFormatter {
    explicit BufferFormatter(const Date& date) : detail::DateFormatter(date){};
};

template <>
struct BufferParser<Date> : detail::DateParser {
    explicit BufferParser(Date& date) : detail::DateParser(date){};
};
}  // namespace cassandra::io
