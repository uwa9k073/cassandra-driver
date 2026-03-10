#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/string_types.hpp>

namespace cassandra::io::detail {

//  [string map]        A [short] n, followed by n pair <k><v> where <k> and <v>
//                      are [string].
//  [string multimap]   A [short] n, followed by n pair <k><v> where <k> is a
//                      [string] and <v> is a [string list].

struct StringMapBinaryParser : BufferParserBase<StringMap> {
    using BaseType = BufferParserBase<StringMap>;
    using BaseType::BaseType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto count = Read<Short>(data, offset);
        this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            auto key = Read<String>(data, offset);
            auto value = Read<String>(data, offset);
            this->value.emplace(std::move(key), std::move(value));
        }
    }
};
struct StringMultimapBinaryParser : BufferParserBase<StringMultiMap> {
    using BaseType = BufferParserBase<StringMultiMap>;
    using BaseType::BaseType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto count = Read<Short>(data, offset);
        this->value.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            auto key = Read<String>(data, offset);
            auto value = Read<StringList>(data, offset);
            this->value.emplace(std::move(key), std::move(value));
        }
    }
};

template <>
struct BufferParser<StringMap> : StringMapBinaryParser {
    explicit BufferParser(StringMap& value) : StringMapBinaryParser(value) {}
};

template <>
struct BufferParser<StringMultiMap> : StringMultimapBinaryParser {
    explicit BufferParser(StringMultiMap& value) : StringMultimapBinaryParser(value) {}
};

}  // namespace cassandra::io::detail
