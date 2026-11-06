#pragma once
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstring>
#include <userver/utils/strong_typedef.hpp>

namespace cassandra::io {

namespace detail {
template <std::size_t>
struct FloatingPointType;

template <>
struct FloatingPointType<4> {
    using type = Float;
    using int_type = Int;
};

template <>
struct FloatingPointType<8> {
    using type = Double;
    using int_type = BigInt;
};

template <std::size_t Size>
struct FloatingPointBySizeParser {
    using FloatType = typename FloatingPointType<Size>::type;
    using IntType = typename FloatingPointType<Size>::int_type;

    static FloatType ParseBuffer(protocol::RawBufferView buf, size_t& offset) {
        const IntType tmp = ReadBuffer<IntType>(buf, offset);
        FloatType float_value{};
        std::memcpy(&float_value, &tmp, Size);
        return float_value;
    }
};

template <std::size_t Size>
struct FloatingPointBySizeFormatter {
    using FloatType = typename FloatingPointType<Size>::type;
    using IntType = typename FloatingPointType<Size>::int_type;

    static void FormatBuffer(protocol::RawBuffer& buf, FloatType value) {
        IntType tmp{};
        std::memcpy(&tmp, &value, Size);
        WriteBuffer<IntType>(buf, tmp);
    }
};

template <class T>
struct FloatingPointBinaryParser : detail::BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;

    void operator()(protocol::RawBufferView buf, size_t& offset) {
        this->value = FloatingPointBySizeParser<sizeof(T)>::ParseBuffer(buf, offset);
    }
};

template <class T>
struct FloatingPointBinaryFormatter
    : ValueFormattingMixin<IntegralBinaryFormatter<T>> {
    using Mixin = ValueFormattingMixin<IntegralBinaryFormatter<T>>;

    using Mixin::operator();

    T _value;

    explicit FloatingPointBinaryFormatter(T value) : _value(value) {}

    void operator()(protocol::RawBuffer& buf) {
        FloatingPointBySizeFormatter<sizeof(T)>::FormatBuffer(buf, _value);
    }

    void operator()(Bytes& buffer) {
        Bytes::UnderlyingType buf;
        FloatingPointBySizeFormatter<sizeof(T)>::FormatBuffer(buf, _value);
        buffer.payload = std::move(buf);
    }
};
}  // namespace detail
template <>
struct BufferParser<Float> : detail::FloatingPointBinaryParser<Float> {
    explicit BufferParser(Float& value) : FloatingPointBinaryParser(value) {}
};

template <>
struct BufferParser<Double> : detail::FloatingPointBinaryParser<Double> {
    explicit BufferParser(Double& value) : FloatingPointBinaryParser(value) {}
};

template <>
struct BufferFormatter<Float> : detail::FloatingPointBinaryFormatter<Float> {
    explicit BufferFormatter(Float value) : FloatingPointBinaryFormatter(value) {}
};

template <>
struct BufferFormatter<Double> : detail::FloatingPointBinaryFormatter<Double> {
    explicit BufferFormatter(Double value) : FloatingPointBinaryFormatter(value) {}
};

}  // namespace cassandra::io
