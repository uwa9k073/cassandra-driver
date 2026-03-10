#pragma once

#include <cassandra/io/protocol/types.hpp>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <userver/utils/void_t.hpp>

namespace cassandra::io::detail {
template <typename T>
struct BufferParserBase {
    using ValueType = T;

    ValueType& value;
    explicit BufferParserBase(ValueType& v) : value{v} {}
};
// for BufferWriter::Read
template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferParser;

// for BufferWriter::Write
template <typename T>
struct BufferFormatter;

inline void EnsureSize(size_t offset, size_t required, size_t data_size) {
    if (offset + required > data_size) {
        throw std::runtime_error("Buffer underread");
    }
}

template <std::integral T>
[[nodiscard]] T ReadIntBE(std::span<const std::byte> data, size_t& offset) {
    EnsureSize(offset, sizeof(T), data.size());
    T value = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        value = (value << 8) | static_cast<std::make_unsigned_t<T>>(data[offset + i]);
    }
    offset += sizeof(T);
    return value;
}

template <std::integral T>
void WriteIntBE(protocol::RawBuffer& data, T value) {
    auto size = sizeof(T);
    if (data.capacity() < data.size() + size) {
        data.reserve(data.size() + size);
    }
    for (size_t i = 0; i < size; ++i) {
        data.push_back(static_cast<std::byte>((value >> ((size - i - 1) * 8)) & 0xFF));
    }
}

template <class T>
[[nodiscard]] T Read(std::span<const std::byte> data, size_t& offset) {
    T val;
    BufferParser<T> parser(val);
    parser(data, offset);
    return val;
}

template <class T>
void Write(protocol::RawBuffer& data, const T& value) {
    BufferFormatter<T> formatter(value);
    formatter(data);
}

template <class T>
void Write(protocol::RawBuffer& data, T&& value) {
    BufferFormatter<T> formatter(value);
    formatter(data);
}

}  // namespace cassandra::io::detail
