#pragma once

#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstring>
#include <type_traits>
#include "cassandra/exception.hpp"

namespace cassandra::io {

struct NullTag {
    auto operator<=>(const NullTag&) const = default;
};

constexpr NullTag kNullTag{};

struct Bytes {
    using SizeType = Int;
    using UnderlyingType = protocol::RawBuffer;
    using Payload = std::variant<NullTag, UnderlyingType>;

    auto operator<=>(const Bytes&) const = default;
    Payload payload;
};

struct ShortBytes {
    using SizeType = Short;
    using UnderlyingType = protocol::RawBuffer;

    UnderlyingType payload;
};

namespace detail {
struct BytesParser : BufferParserBase<Bytes> {
    using BaseType = BufferParserBase<Bytes>;
    using BaseType::BaseType;
    using SizeType = Bytes::SizeType;
    using Underlying = Bytes::UnderlyingType;

    void operator()(protocol::RawBufferView buffer, size_t& offset) {
        SizeType size = ReadBuffer<SizeType>(buffer, offset);
        if (size == -1) {
            this->value.payload = kNullTag;
        } else {
            Underlying val;
            val.resize(size);
            std::memcpy(val.data(), buffer.data() + offset, size);
            this->value.payload = val;
            offset += size;
        }
    }
};

struct BytesFormatter : BufferFormatterBase<Bytes> {
    using BaseType = BufferFormatterBase<Bytes>;
    using BaseType::BaseType;
    using SizeType = Bytes::SizeType;
    using Underlying = Bytes::UnderlyingType;

    void operator()(protocol::RawBuffer& buffer) {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Underlying>) {
                    SizeType size = arg.size();
                    WriteBuffer<SizeType>(buffer, size);
                    SizeType offset = buffer.size();
                    buffer.resize(offset + size);
                    std::memcpy(buffer.data() + offset, arg.data(), size);
                } else if constexpr (std::is_same_v<T, NullTag>) {
                    WriteBuffer<SizeType>(buffer, -1);
                }
            },
            this->value.payload
        );
    }
};

struct ShortBytesParser : BufferParserBase<ShortBytes> {
    using BaseType = BufferParserBase<ShortBytes>;
    using BaseType::BaseType;
    using SizeType = Bytes::SizeType;
    using Underlying = Bytes::UnderlyingType;

    void operator()(protocol::RawBufferView buffer, size_t& offset) {
        SizeType size = ReadBuffer<SizeType>(buffer, offset);
        Underlying val;
        val.resize(size);
        std::memcpy(val.data(), buffer.data() + offset, size);
        this->value.payload = val;
        offset += size;
    }
};

struct ShortBytesFormatter : BufferFormatterBase<ShortBytes> {
    using BaseType = BufferFormatterBase<ShortBytes>;
    using BaseType::BaseType;
    using SizeType = Bytes::SizeType;
    using Underlying = Bytes::UnderlyingType;
    void operator()(protocol::RawBuffer& buffer) {
        SizeType size = this->value.payload.size();
        WriteBuffer<SizeType>(buffer, this->value.payload.size());
        SizeType offset = buffer.size();
        buffer.resize(offset + size);
        std::memcpy(buffer.data() + offset, this->value.payload.data(), size);
    }
};
}  // namespace detail

template <>
struct BufferParser<Bytes> : detail::BytesParser {
    explicit BufferParser(Bytes& val) : detail::BytesParser(val) {}
};

template <>
struct BufferFormatter<Bytes> : detail::BytesFormatter {
    explicit BufferFormatter(const Bytes& val) : detail::BytesFormatter(val) {}
};

template <>
struct BufferParser<ShortBytes> : detail::ShortBytesParser {
    explicit BufferParser(ShortBytes& val) : detail::ShortBytesParser(val) {}
};

template <>
struct BufferFormatter<ShortBytes> : detail::ShortBytesFormatter {
    explicit BufferFormatter(const ShortBytes& val)
        : detail::ShortBytesFormatter(val) {}
};

}  // namespace cassandra::io
