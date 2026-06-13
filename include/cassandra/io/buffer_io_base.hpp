#pragma once

#include <boost/endian/conversion.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/traits.hpp>
#include <cstring>
#include <userver/utils/void_t.hpp>

namespace cassandra::io::detail {
template <typename T>
struct BufferParserBase {
    using ValueType = T;

    ValueType& value;
    explicit BufferParserBase(ValueType& v) : value{v} {}
};

template <typename T>
struct BufferParserBase<T&&> {
    using ValueType = T;

    ValueType value;
    explicit BufferParserBase(ValueType&& v) : value{std::move(v)} {}
};

template <typename T>
struct BufferFormatterBase {
    using ValueType = T;
    const ValueType& value;
    explicit BufferFormatterBase(const ValueType& v) : value{v} {}
};
}  // namespace cassandra::io::detail
