#pragma once

#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/traits.hpp>
#include <cassandra/io/value.hpp>
#include <cassandra/io/varint.hpp>
#include <cstddef>
namespace cassandra::io {

/**
 * The decimal format represents an arbitrary-precision number.
 * It contains an [int] "scale" component followed by a varint encoding
 * of the unscaled value. The encoded value represents "<unscale>E<-scale>".
 * In other words, "<unscaled> * 10 ^ (-1 * <scale>)"
 */
struct Decimal {
    Int scale;
    Varint unscaled;

    auto operator<=>(const Decimal&) const = default;
};

namespace detail {
struct DecimalParser : BufferParserBase<Decimal> {
    using BaseType::BaseType;
    void operator()(const Bytes& buffer) {
        std::size_t offset = 0;
        protocol::RawBufferView payload = std::get<1>(buffer.payload);
        this->value.scale = ReadBuffer<Int>(payload, offset);
        this->value.unscaled = ReadBuffer<Varint>(payload, offset);
    }
};
struct DecimalFormatter : BufferFormatterBase<Decimal>,
                          ValueFormattingMixin<DecimalFormatter> {
    using BaseType::BaseType;
    using Mixin::operator();

    void operator()(Bytes& buffer) {
        Bytes::UnderlyingType payload;
        WriteBuffer<Int>(payload, this->value.scale);
        WriteBuffer<Varint>(payload, this->value.unscaled);

        buffer.payload = std::move(payload);
    }
};
}  // namespace detail

template <>
struct BufferParser<Decimal> : detail::DecimalParser {
    using detail::DecimalParser::DecimalParser;
};

template <>
struct BufferFormatter<Decimal> : detail::DecimalFormatter {
    using detail::DecimalFormatter::DecimalFormatter;
};
}  // namespace cassandra::io
