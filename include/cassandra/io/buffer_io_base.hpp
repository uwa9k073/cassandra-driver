#pragma once

#include <boost/endian/conversion.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>
#include <cstring>
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
template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferFormatter;

template <class T>
struct Input {
    using type = BufferParser<T>;
};

template <class T>
struct Output {
    using type = BufferFormatter<T>;
};

template <class T>
struct IO {
    using ParserType = typename Input<T>::type;
    using FormatterType = typename Output<T>::type;
};

inline void EnsureSize(size_t offset, size_t required, size_t data_size) {
    if (offset + required > data_size) {
        throw std::runtime_error("Buffer underread");
    }
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

template <class T>
[[nodiscard]] T Read(protocol::RawBufferView data, size_t& offset) {
    T val;
    using Parser = typename IO<T>::ParserType;
    Parser parser(val);
    parser(data, offset);
    return val;
}

template <class T>
void Write(protocol::RawBuffer& data, const T& value) {
    using Formatter = typename IO<T>::FormatterType;
    Formatter formatter(value);
    formatter(data);
}

template <class It, class T>
void WriteIntBE(It begin, It end, const T value) {
    auto size = sizeof(T);
    if (end - begin < size) {
        throw std::out_of_range("buffer is too small");
    }
    auto tmp = boost::endian::native_to_big(value);
    auto* ptr = reinterpret_cast<std::byte*>(&tmp);
    // auto* end = ptr + size;
    std::memcpy(begin, ptr, size);
}
// template <class T>
// void Write(protocol::RawBuffer& data, T&& value) {
//     using Formatter = typename IO<T>::FormatterType;
//     Formatter formatter(value);
//     formatter(data);
// }

}  // namespace cassandra::io::detail
