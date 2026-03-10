#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cstddef>
#include <string>

namespace cassandra::io::detail {

template <typename T>
struct StringBinaryParser : BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto size = ReadIntBE<T::TagType>(data, offset);
        std::string res(reinterpret_cast<const char*>(data.data()) + offset, size);
        offset += size;
        this->value = T{res};
    }
};

template <>
struct BufferParser<String> : StringBinaryParser<String> {
    explicit BufferParser(String& val) : StringBinaryParser(val) {}
};

template <>
struct BufferParser<LongString> : StringBinaryParser<LongString> {
    explicit BufferParser(LongString& val) : StringBinaryParser(val) {}
};
}  // namespace cassandra::io::detail
