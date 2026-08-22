#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/string_types.hpp>
#include "cassandra/io/buffer_io.hpp"
#include "cassandra/io/bytes.hpp"
#include "cassandra/io/protocol/types.hpp"

namespace cassandra::io {

//  [string map]        A [short] n, followed by n pair <k><v> where
//  <k> and <v>
//                      are [string].
//  [string multimap]   A [short] n, followed by n pair <k><v> where
//  <k> is a
//                      [string] and <v> is a [string list].
namespace detail {
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

template <concepts::MapConcept Map, size_t Size = sizeof(Int)>
struct MapBinaryParser : BufferParserBase<Map> {
    using BaseType = BufferParserBase<Map>;
    using BaseType::BaseType;

    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;
    using LenType = typename MapLenBySize<Size>::type;

    void operator()(protocol::RawBufferView data, size_t& offset) {
        size_t count = ReadBuffer<LenType>(data, offset);

        for (size_t i = 0; i < count; ++i) {
            auto key = ReadBuffer<KeyType>(data, offset);
            auto value = ReadBuffer<MappedType>(data, offset);
            this->value.emplace(std::move(key), std::move(value));
        }
    }

    void operator()(const Bytes& bytes) {
        protocol::RawBufferView payload = std::get<1>(bytes.payload);
        std::size_t offset = 0;
        auto size = ReadBuffer<LenType>(payload, offset);

        for (size_t i = 0; i < size; ++i) {
            auto key = ReadBuffer<Bytes>(payload, offset);
            auto value = ReadBuffer<Bytes>(payload, offset);
            this->value.emplace(
                ReadBuffer<KeyType>(key), ReadBuffer<MappedType>(value)
            );
        }
    }
};

template <concepts::MapConcept Map, size_t Size = sizeof(Int)>
struct MapBinaryFormatter {
    using LenType = typename ListLenBySize<Size>::type;
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;

    const Map& value;
    explicit MapBinaryFormatter(const Map& value) : value(value) {}
    void operator()(protocol::RawBuffer& data) const {
        WriteBuffer<LenType>(data, value.size());
        for (const auto& [key, val] : value) {
            WriteBuffer<KeyType>(data, key);
            WriteBuffer<MappedType>(data, val);
        }
    }
};

}  // namespace detail

namespace traits {
template <>
struct Input<StringMap> {
    using type = detail::MapBinaryParser<StringMap, 2>;
};

template <>
struct Input<StringMultiMap> {
    using type = detail::MapBinaryParser<StringMultiMap, 2>;
};

template <>
struct Output<StringMap> {
    using type = detail::MapBinaryFormatter<StringMap, 2>;
};

template <>
struct Output<StringMultiMap> {
    using type = detail::MapBinaryFormatter<StringMultiMap, 2>;
};

template <concepts::MapConcept Map>
struct Input<Map> {
    using type = detail::MapBinaryParser<Map>;
};

template <concepts::MapConcept Map>
struct Output<Map> {
    using type = detail::MapBinaryFormatter<Map>;
};

template <>
struct Input<BytesMap> {
    using type = detail::MapBinaryParser<BytesMap, 2>;
};

template <>
struct Output<BytesMap> {
    using type = detail::MapBinaryFormatter<BytesMap, 2>;
};
}  // namespace traits
}  // namespace cassandra::io
