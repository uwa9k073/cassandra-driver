#pragma once
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/string_types.hpp>
#include <iterator>
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

template <SequenceContainerConcept Container>
struct ListBinaryParser : BufferParserBase<Container> {
    using BaseType = BufferParserBase<Container>;
    using BaseType::BaseType;
    using ElementType = typename Container::value_type;
    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto count = Read<Int>(data, offset);
        this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            auto inserter = std::inserter(this->value, this->value.end());
            *inserter = Read<ElementType>(data, offset);
        }
    }
};

struct StringListBinaryParser : BufferParserBase<StringList> {
    using BaseType = BufferParserBase<StringList>;
    using BaseType::BaseType;
    using ElementType = String;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto count = Read<Short>(data, offset);
        this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            this->value.push_back(Read<ElementType>(data, offset));
        }
    }
};

// template<class T>
// struct Input<T>;

template <>
struct BufferParser<StringList> : StringListBinaryParser {
    explicit BufferParser(StringList& value) : StringListBinaryParser(value) {}
};
}  // namespace cassandra::io::detail
