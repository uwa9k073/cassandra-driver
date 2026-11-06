#pragma once
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/concepts.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/string_types.hpp>
#include <concepts>
#include "cassandra/io/bytes.hpp"
namespace cassandra::io {

//  [list]          A [int] n indicating the number of elements in the
//  list, followed by n
//                  elements.  Each element is [bytes] representing
//                  the serialized value.
//  [bytes]         A [int] n, followed by n bytes if n >= 0. If n < 0,
//                  no byte should follow and the value represented is `null`.
//  [string list]   A [short] n, followed by n [string].
//  [short bytes]   A [short] n, followed by n bytes if n >= 0.
//

namespace detail {

template <size_t Size>
struct ListLenBySize;

template <>
struct ListLenBySize<2> {
    using type = Short;
};

template <>
struct ListLenBySize<4> {
    using type = Int;
};

template <concepts::SequenceContainerConcept Container, size_t Size = sizeof(Int)>
struct ListBinaryParser : BufferParserBase<Container> {
    using BaseType = BufferParserBase<Container>;
    using BaseType::BaseType;

    using ElementType = typename Container::value_type;
    using SizeType = typename ListLenBySize<Size>::type;
    // Notation list:
    void operator()(protocol::RawBufferView data, size_t& offset) {
        size_t count = ReadBuffer<SizeType>(data, offset);
        // this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            this->value.push_back(ReadBuffer<ElementType>(data, offset));
        }
    }

    // Column list:
    // A [int] n indicating the number of elements in the list, followed by n
    // elements.  Each element is [bytes] representing the serialized value.
    void operator()(const Bytes& buffer) {
        protocol::RawBufferView payload = std::get<1>(buffer.payload);
        std::size_t offset = 0;
        auto count = ReadBuffer<SizeType>(payload, offset);
        this->value.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            // each element is [bytes] representing the serialized value
            //  so we read len of [bytes] and then memcpy the needed data by that
            //  length to cassandra::io::Bytes, and after that we read element from
            //  [bytes]
            auto element_size = ReadBuffer<SizeType>(payload, offset);
            this->value.push_back(ReadBuffer<ElementType>(
                Bytes{.payload = payload.subspan(offset, element_size)}
            ));
            offset += element_size;
        }
    }
};

template <class T>
concept StrongTypedefConcept = requires(T value) {
    typename T::TagType;
    typename T::UnderlyingType;

    { value.GetUnderlying() } -> std::convertible_to<typename T::UnderlyingType&>;
};

template <concepts::SequenceContainerConcept Container, size_t Size = sizeof(Int)>
struct ListBinaryFormatter : BufferFormatterBase<Container> {
    using ElementType = typename Container::value_type;
    using SizeType = typename ListLenBySize<Size>::type;

    using BaseType = BufferFormatterBase<Container>;
    using BaseType::BaseType;

    void operator()(protocol::RawBuffer& buffer) {
        WriteBuffer<SizeType>(buffer, this->value.size());
        for (const auto& elem : this->value) {
            WriteBuffer<ElementType>(buffer, elem);
        }
    }

    // Column list:
    // A [int] n indicating the number of elements in the list, followed by n
    // elements.  Each element is [bytes] representing the serialized value.
    void operator()(Bytes& buffer) {
        Bytes::UnderlyingType underlying;
        WriteBuffer<SizeType>(underlying, this->value.size());
        for (const auto& elem : this->value) {
            Bytes bytes;
            WriteBuffer<ElementType>(bytes, elem);
            WriteBuffer<Bytes>(underlying, bytes);
        }

        buffer.payload = std::move(underlying);
    }
};

}  // namespace detail

namespace traits {
template <concepts::SequenceContainerConcept Container>
struct Input<Container> {
    using type = detail::ListBinaryParser<Container>;
};

template <concepts::SequenceContainerConcept Container>
struct Output<Container> {
    using type = detail::ListBinaryFormatter<Container>;
};

template <>
struct Input<StringList> {
    using type = detail::ListBinaryParser<StringList, 2>;
};

template <>
struct Output<StringList> {
    using type = detail::ListBinaryFormatter<StringList, 2>;
};
}  // namespace traits
}  // namespace cassandra::io
