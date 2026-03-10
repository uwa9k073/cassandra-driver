#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>

namespace cassandra::io::detail {

template <typename T>
struct StringBinaryParser : BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;
    using SizeType = typename T::TagType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto size = Read<SizeType>(data, offset);
        typename T::UnderlyingType res(reinterpret_cast<const char*>(data.data()) + offset, size);
        offset += size;
        this->value = T{res};
    }
};

template <typename T>
struct StringBinaryFormatter {
    using SizeType = typename T::TagType;
    T value;
    explicit StringBinaryFormatter(T val) : value(val) {}

    void operator()(protocol::RawBuffer& buffer) const {
        auto size = static_cast<SizeType>(value.GetUnderlying().size());
        auto total_size = size + sizeof(SizeType);
        buffer.reserve(buffer.size() + total_size);
        Write<SizeType>(buffer, size);
        std::memcpy(buffer.data() + size, value.GetUnderlying(), size);
        buffer.resize(buffer.size() + size);
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
