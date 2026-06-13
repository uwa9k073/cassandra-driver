#pragma once
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/string_types.hpp>
#include <concepts>
#include "cassandra/io/protocol/types.hpp"
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
template <typename T>
concept SequenceContainerConcept = requires(T container) {
    typename T::value_type;

    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;

    { container.begin() } -> std::same_as<typename T::iterator>;
    { container.end() } -> std::same_as<typename T::iterator>;
    { container.cbegin() } -> std::same_as<typename T::const_iterator>;
    { container.cend() } -> std::same_as<typename T::const_iterator>;
    { container.size() } -> std::convertible_to<typename T::size_type>;
    { container.empty() } -> std::convertible_to<bool>;

    { container.front() } -> std::same_as<typename T::value_type&>;
    { container.back() } -> std::same_as<typename T::value_type&>;
};

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

template <SequenceContainerConcept Container, size_t Size = sizeof(Int)>
struct ListBinaryParser : BufferParserBase<Container> {
    using BaseType = BufferParserBase<Container>;
    using BaseType::BaseType;

    using ElementType = typename Container::value_type;
    using LenType = typename ListLenBySize<Size>::type;

    void operator()(protocol::RawBufferView data, size_t& offset) {
        size_t count = ReadBuffer<LenType>(data, offset);
        // this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            this->value.push_back(ReadBuffer<ElementType>(data, offset));
        }
    }
};

template <class T>
concept StrongTypedefConcept = requires(T value) {
    typename T::TagType;
    typename T::UnderlyingType;

    { value.GetUnderlying() } -> std::convertible_to<typename T::UnderlyingType&>;
};

template <StrongTypedefConcept BytesStrongTypedef>
struct BytesBinaryParser : BufferParserBase<BytesStrongTypedef> {
    using BaseType = BufferParserBase<BytesStrongTypedef>;
    using BaseType::BaseType;
    using SizeType = typename BytesStrongTypedef::TagType;
    using UnderlyingType = typename BytesStrongTypedef::UnderlyingType;

    void operator()(protocol::RawBufferView data, size_t& offset) {
        SizeType len = ReadBuffer<SizeType>(data, offset);
        if (len > 0) {
            UnderlyingType underlying;
            underlying.resize(len);
            std::memcpy(underlying.data(), data.data() + offset, len);
            this->value = BytesStrongTypedef{std::move(underlying)};
            offset += len;
        } else {
            this->value = {};
        }
    }
};

template <StrongTypedefConcept BytesStrongTypedef>
struct BytesBinaryFormatter {
    const BytesStrongTypedef& value;
    using SizeType = typename BytesStrongTypedef::TagType;

    void operator()(protocol::RawBuffer& buffer) {
        SizeType size = value.GetUnderlying().size();
        WriteBuffer<SizeType>(buffer, size);
        if (size <= 0) {
            return;
        }
        auto offset = buffer.size();
        buffer.resize(buffer.size() + size);
        std::memcpy(buffer.data() + offset, value.GetUnderlying().data(), size);
    }
};

template <SequenceContainerConcept Container, size_t Size = sizeof(Int)>
struct ListBinaryFormatter {
    const Container& value;

    using ElementType = typename Container::value_type;
    using LenType = typename ListLenBySize<Size>::type;

    explicit ListBinaryFormatter(const Container& value) : value(value) {}

    void operator()(protocol::RawBuffer& buffer) {
        WriteBuffer<LenType>(buffer, value.size());
        for (const auto& elem : value) {
            WriteBuffer<ElementType>(buffer, elem);
        }
    }
};

}  // namespace detail

namespace traits {
template <detail::SequenceContainerConcept Container>
struct Input<Container> {
    using type = detail::ListBinaryParser<Container>;
};

template <detail::SequenceContainerConcept Container>
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

template <>
struct Input<ShortBytes> {
    using type = detail::BytesBinaryParser<ShortBytes>;
};

template <>
struct Output<ShortBytes> {
    using type = detail::BytesBinaryFormatter<ShortBytes>;
};

template <>
struct Input<Bytes> {
    using type = detail::BytesBinaryParser<Bytes>;
};

template <>
struct Output<Bytes> {
    using type = detail::BytesBinaryFormatter<Bytes>;
};
}  // namespace traits
}  // namespace cassandra::io
