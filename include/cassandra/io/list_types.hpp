#pragma once
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>

namespace cassandra::io::detail {

//   [list]     A [int] n indicating the number of elements in the list, followed by n
//              elements.  Each element is [bytes] representing the serialized value.
//   [bytes]    A [int] n, followed by n bytes if n >= 0. If n < 0,
//              no byte should follow and the value represented is `null`.

// template<class T>
// struct ListParser : BufferParserBase<T> {
//     using BaseType = BufferParserBase<T>;
//     using BaseType::BaseType;
//     void operator()(std::span<const std::byte> data, size_t& offset) {
//         auto count = ReadIntBE<Int>(data, offset);
//         T list;
//         list.reserve(count);
//         for (size_t i = 0; i < count; ++i) {
//             T
//         }
//     }
// };
}  // namespace cassandra::io::detail
