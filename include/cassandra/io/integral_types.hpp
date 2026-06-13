#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>

namespace cassandra::io {
namespace detail {

inline void EnsureSize(size_t offset, size_t required, size_t data_size) {
    if (offset + required > data_size) {
        throw std::runtime_error("Buffer underread");
    }
}

template <class It, class T>
void WriteIntBE(It begin, It end, const T& value) {
    size_t size = sizeof(T);
    if ((size_t)(end - begin) < size) {
        throw std::out_of_range("buffer is too small");
    }
    auto tmp = boost::endian::native_to_big(value);
    auto* ptr = reinterpret_cast<std::byte*>(&tmp);
    // auto* end = ptr + size;
    std::memcpy(begin, ptr, size);
}

template <std::integral T>
[[nodiscard]] T ReadIntBE(protocol::RawBufferView data, size_t& offset) {
    EnsureSize(offset, sizeof(T), data.size());
    T value;
    std::memcpy(&value, data.data() + offset, sizeof(T));
    offset += sizeof(T);
    return boost::endian::big_to_native(value);
}

template <std::integral T>
void WriteIntBE(protocol::RawBuffer& data, T value) {
    auto size = sizeof(T);
    if (data.capacity() < data.size() + size) {
        data.reserve(data.size() + size);
    }
    auto tmp = boost::endian::native_to_big(value);
    auto* ptr = reinterpret_cast<std::byte*>(&tmp);
    auto* end = ptr + size;
    std::copy(ptr, end, std::back_inserter(data));
}

template <typename T>
struct IntegralBinaryParser : BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        this->value = ReadIntBE<T>(data, offset);
    }

    void operator()(const Bytes& buffer) {
        size_t offset = 0;
        this->value = ReadIntBE<T>(buffer.GetUnderlying(), offset);
    }
};

template <typename T>
struct IntegralBinaryFormatter {
    T value;
    explicit IntegralBinaryFormatter(T value) : value(value) {}
    void operator()(protocol::RawBuffer& buffer) { WriteIntBE(buffer, this->value); }

    void operator()(Bytes& buffer) {
        WriteIntBE(buffer.GetUnderlying(), this->value);
    }
};
}  // namespace detail

template <>
struct BufferParser<Boolean> : detail::IntegralBinaryParser<Boolean> {
    explicit BufferParser(Boolean& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Boolean> : detail::IntegralBinaryFormatter<Boolean> {
    explicit BufferFormatter(Boolean val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<Byte> : detail::IntegralBinaryParser<Byte> {
    explicit BufferParser(Byte& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Byte> : detail::IntegralBinaryFormatter<Byte> {
    explicit BufferFormatter(Byte val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<TinyInt> : detail::IntegralBinaryParser<TinyInt> {
    explicit BufferParser(TinyInt& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<TinyInt> : detail::IntegralBinaryFormatter<TinyInt> {
    explicit BufferFormatter(TinyInt val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<SmallInt> : detail::IntegralBinaryParser<SmallInt> {
    explicit BufferParser(SmallInt& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<SmallInt> : detail::IntegralBinaryFormatter<SmallInt> {
    explicit BufferFormatter(SmallInt val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<Int> : detail::IntegralBinaryParser<Int> {
    explicit BufferParser(Int& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Int> : detail::IntegralBinaryFormatter<Int> {
    explicit BufferFormatter(Int val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<BigInt> : detail::IntegralBinaryParser<BigInt> {
    explicit BufferParser(BigInt& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<BigInt> : detail::IntegralBinaryFormatter<BigInt> {
    explicit BufferFormatter(BigInt val) : IntegralBinaryFormatter(val) {}
};

template <>
struct BufferParser<Short> : detail::IntegralBinaryParser<Short> {
    explicit BufferParser(Short& val) : IntegralBinaryParser(val) {}
};

template <>
struct BufferFormatter<Short> : detail::IntegralBinaryFormatter<Short> {
    explicit BufferFormatter(Short val) : IntegralBinaryFormatter(val) {}
};

}  // namespace cassandra::io
