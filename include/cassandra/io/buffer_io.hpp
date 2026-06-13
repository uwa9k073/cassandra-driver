#pragma once

#include <boost/endian/conversion.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/traits.hpp>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <userver/utils/void_t.hpp>

namespace cassandra::io {

template <class T>
[[nodiscard]] T ReadBuffer(protocol::RawBufferView data, size_t& offset) {
    T val;
    using Parser = typename traits::IO<T>::ParserType;
    Parser parser(val);
    parser(data, offset);
    return val;
}

template <class T>
[[nodiscard]] T ReadBuffer(const Bytes& data)
    requires std::same_as<Bytes, std::decay_t<decltype(data)>>
{
    T val;
    using Parser = typename traits::IO<T>::ParserType;
    Parser parser(val);
    parser(data);
    return val;
}

template <class T>
void WriteBuffer(protocol::RawBuffer& data, const T& value) {
    using Formatter = typename traits::IO<T>::FormatterType;
    Formatter formatter(value);
    formatter(data);
}

template <class T>
void WriteBuffer(Bytes& data, const T& value) {
    using Formatter = typename traits::IO<T>::FormatterType;
    Formatter formatter(value);
    formatter(data);
}

}  // namespace cassandra::io
