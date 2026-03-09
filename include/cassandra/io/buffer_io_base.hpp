#pragma once

#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <userver/utils/void_t.hpp>

namespace cassandra::io::detail {
template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferParser;

template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferFormatter;

template <typename T>
[[nodiscard]] T Parse(BufferReader& reader) {
    return BufferParser<T>{}(reader);
}

// Parse into existing object
template <typename T>
void ParseInto(BufferReader& reader, T& out) {
    out = BufferParser<T>{}(reader);
}

// template <typename T>
// void Format(BufferWriter& writer, const T& value) {
//     BufferFormatter<T>{}(writer, value);
// }
}  // namespace cassandra::io::detail
