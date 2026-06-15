#pragma once

#include <userver/utils/void_t.hpp>
namespace cassandra::io {
// for BufferWriter::Read
template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferParser;

// for BufferWriter::Write
template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferFormatter;

namespace traits {
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
}  // namespace traits
}  // namespace cassandra::io
