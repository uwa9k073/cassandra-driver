#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cstddef>
#include <cassandra/io/protocol/types.hpp>

namespace cassandra::io::detail {

template <typename T>
struct IntegralBinaryParser : BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;

    void operator()(std::span<const std::byte> data, size_t& offset) { this->value = ReadIntBE<T>(data, offset); }
};

template <typename T>
struct IntegralBinaryFormatter {
    T value;
    explicit IntegralBinaryFormatter(T value) : value(value) {}
    void operator()(protocol::RawBuffer& buffer) { WriteIntBE(buffer, this->value); }
};

template <>
struct BufferParser<TinyInt> : detail::IntegralBinaryParser<TinyInt> {
    explicit BufferParser(TinyInt& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<TinyInt> : detail::IntegralBinaryFormatter<TinyInt> {
    explicit BufferFormatter(TinyInt& val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<SmallInt> : detail::IntegralBinaryParser<SmallInt> {
    explicit BufferParser(SmallInt& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<SmallInt> : detail::IntegralBinaryFormatter<SmallInt> {
    explicit BufferFormatter(SmallInt& val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<Int> : detail::IntegralBinaryParser<Int> {
    explicit BufferParser(Int& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Int> : detail::IntegralBinaryFormatter<Int> {
    explicit BufferFormatter(Int& val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<BigInt> : detail::IntegralBinaryParser<BigInt> {
    explicit BufferParser(BigInt& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<BigInt> : detail::IntegralBinaryFormatter<BigInt> {
    explicit BufferFormatter(BigInt& val) : IntegralBinaryFormatter(val) {}
};

}  // namespace cassandra::io::detail
