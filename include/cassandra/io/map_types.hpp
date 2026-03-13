#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/string_types.hpp>

namespace cassandra::io::detail {

//  [string map]        A [short] n, followed by n pair <k><v> where
//  <k> and <v>
//                      are [string].
//  [string multimap]   A [short] n, followed by n pair <k><v> where
//  <k> is a
//                      [string] and <v> is a [string list].

template <size_t Size>
struct MapLenBySize;

template <>
struct MapLenBySize<2> {
    using type = Short;
};

template <>
struct MapLenBySize<4> {
    using type = Int;
};

template <typename T>
concept MapConcept =
    requires(T container) {
        typename T::value_type;
        typename T::key_type;
        typename T::mapped_type;

        typename T::iterator;
        typename T::const_iterator;
        typename T::size_type;

        { container.begin() } -> std::same_as<typename T::iterator>;
        { container.end() } -> std::same_as<typename T::iterator>;
        { container.cbegin() } -> std::same_as<typename T::const_iterator>;
        { container.cend() } -> std::same_as<typename T::const_iterator>;
        { container.size() } -> std::convertible_to<typename T::size_type>;
        { container.empty() } -> std::convertible_to<bool>;
        {
            typename T::value_type{}
        } -> std::convertible_to<
              std::pair<const typename T::key_type, typename T::mapped_type>>;
    } &&
    (
        // --- Emplace Logic: OR Condition ---
        // Case 1: Associative Maps (returns iterator)
        requires(T container) {
            {
                container.emplace(
                    std::declval<typename T::key_type>(),
                    std::declval<typename T::mapped_type>()
                )
            } -> std::same_as<typename T::iterator>;
        } ||
        // Case 2: Unordered Associative Maps (returns pair<iterator,
        // bool>)
        requires(T container) {
            {
                container.emplace(
                    std::declval<typename T::key_type>(),
                    std::declval<typename T::mapped_type>()
                )
            } -> std::same_as<std::pair<typename T::iterator, bool>>;
        }
    );

template <MapConcept Map, size_t Size = sizeof(Int)>
struct MapBinaryParser : BufferParserBase<Map> {
    using BaseType = BufferParserBase<Map>;
    using BaseType::BaseType;

    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;
    using LenType = typename MapLenBySize<Size>::type;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        size_t count = Read<LenType>(data, offset);

        for (size_t i = 0; i < count; ++i) {
            auto key = Read<KeyType>(data, offset);
            auto value = Read<MappedType>(data, offset);
            this->value.emplace(std::move(key), std::move(value));
        }
    }
};

template <MapConcept Map, size_t Size = sizeof(Int)>
struct MapBinaryFormatter {
    using LenType = typename ListLenBySize<Size>::type;
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;

    const Map& value;
    explicit MapBinaryFormatter(const Map& value) : value(value) {}
    void operator()(protocol::RawBuffer& data) const {
        Write<LenType>(data, value.size());
        for (const auto& [key, val] : value) {
            Write<KeyType>(data, key);
            Write<MappedType>(data, val);
        }
    }
};

template <>
struct Input<StringMap> {
    using type = MapBinaryParser<StringMap, 2>;
};

template <>
struct Input<StringMultiMap> {
    using type = MapBinaryParser<StringMultiMap, 2>;
};

template <>
struct Output<StringMap> {
    using type = MapBinaryFormatter<StringMap, 2>;
};

template <>
struct Output<StringMultiMap> {
    using type = MapBinaryFormatter<StringMultiMap, 2>;
};

template <MapConcept Map>
struct Input<Map> {
    using type = MapBinaryParser<Map>;
};

template <MapConcept Map>
struct Output<Map> {
    using type = MapBinaryFormatter<Map>;
};

template <>
struct Input<BytesMap> {
    using type = MapBinaryParser<BytesMap, 2>;
};

template <>
struct Output<BytesMap> {
    using type = MapBinaryFormatter<BytesMap, 2>;
};
}  // namespace cassandra::io::detail
