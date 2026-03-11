#pragma once
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/string_types.hpp>
#include <concepts>
#include "cassandra/io/protocol/types.hpp"
namespace cassandra::io::detail {

//  [list]          A [int] n indicating the number of elements in the list, followed by n
//                  elements.  Each element is [bytes] representing the serialized value.
//  [bytes]         A [int] n, followed by n bytes if n >= 0. If n < 0,
//                  no byte should follow and the value represented is `null`.
//  [string list]   A [short] n, followed by n [string].
//  [short bytes]   A [short] n, followed by n bytes if n >= 0.
//
//
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

    void operator()(std::span<const std::byte> data, size_t& offset) {
        size_t count = Read<LenType>(data, offset);
        // this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            this->value.push_back(Read<ElementType>(data, offset));
        }
    }
};

template <SequenceContainerConcept Container, size_t Size = sizeof(Int)>
struct ListBinaryFormatter {
    const Container& value;

    using ElementType = typename Container::value_type;
    using LenType = typename ListLenBySize<Size>::type;

    explicit ListBinaryFormatter(const Container& value) : value(value) {}

    void operator()(protocol::RawBuffer& buffer) {
        Write<LenType>(buffer, value.size());
        for (const auto& elem : value) {
            Write<ElementType>(buffer, elem);
        }
    }
};

template <SequenceContainerConcept Container>
struct Input<Container> {
    using type = ListBinaryParser<Container>;
};

template <SequenceContainerConcept Container>
struct Output<Container> {
    using type = ListBinaryFormatter<Container>;
};

template <>
struct Input<StringList> {
    using type = ListBinaryParser<StringList, 2>;
};

template <>
struct Output<StringList> {
    using type = ListBinaryFormatter<StringList, 2>;
};

}  // namespace cassandra::io::detail
