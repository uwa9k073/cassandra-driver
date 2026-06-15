#pragma once

#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>
#include <cstring>
#include <userver/utils/strong_typedef.hpp>
#include "cassandra/io/value.hpp"

namespace cassandra::io {
namespace detail {

template <typename T>
struct StringBinaryParser : BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;
    using SizeType = typename T::TagType;
    using Underlying = typename T::UnderlyingType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto size = ReadBuffer<SizeType>(data, offset);
        Underlying res;
        res.reserve(size);
        res.resize(size);
        std::memcpy(res.data(), data.data() + offset, size);
        offset += size;
        this->value = std::move(T{res});
    }
};

struct CommonStringBinaryParser : BufferParserBase<std::string> {
    using BaseType = BufferParserBase<std::string>;
    using BaseType::BaseType;
    using SizeType = Short;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto size = ReadBuffer<SizeType>(data, offset);
        this->value.reserve(size);
        this->value.resize(size);
        std::memcpy(
            this->value.data(),
            reinterpret_cast<const char*>(data.data()) + offset,
            size
        );
        offset += size;
    }

    void operator()(const Bytes& buffer) {
        const auto& raw_buffer_payload =
            std::get<Bytes::UnderlyingType>(buffer.payload);
        auto size = raw_buffer_payload.size();
        this->value.reserve(size);
        this->value.resize(size);
        std::memcpy(
            this->value.data(),
            reinterpret_cast<const char*>(raw_buffer_payload.data()),
            size
        );
    }
};

template <typename T>
struct StringBinaryFormatter {
    using SizeType = typename T::TagType;
    const T& value;
    explicit StringBinaryFormatter(const T& val) : value(val) {}

    void operator()(protocol::RawBuffer& buffer) const {
        auto size = static_cast<SizeType>(value.GetUnderlying().size());
        WriteBuffer<SizeType>(buffer, size);
        auto offset = buffer.size();
        buffer.reserve(offset + size);
        buffer.resize(offset + size);
        std::memcpy(
            buffer.data() + offset,
            reinterpret_cast<const std::byte*>(value.GetUnderlying().data()),
            size
        );
    }
};

struct CommonStringBinaryFormatter
    : BufferFormatterBase<std::string>,
      ValueFormattingMixin<CommonStringBinaryFormatter> {
    using SizeType = Short;
    using BaseType = BufferFormatterBase<std::string>;
    using BaseType::BaseType;

    using Mixin = ValueFormattingMixin<CommonStringBinaryFormatter>;
    using Mixin::operator();

    void operator()(protocol::RawBuffer& buffer) const {
        auto size = static_cast<SizeType>(value.size());
        WriteBuffer<SizeType>(buffer, size);
        auto offset = buffer.size();
        buffer.reserve(offset + size);
        buffer.resize(offset + size);
        std::memcpy(
            buffer.data() + offset,
            reinterpret_cast<const std::byte*>(value.data()),
            size
        );
    }

    void operator()(Bytes& buffer) const {
        using Type = Bytes::UnderlyingType;
        Type dest;
        dest.resize(this->value.size());
        std::memcpy(
            dest.data(),
            reinterpret_cast<const std::byte*>(value.data()),
            value.size()
        );
        buffer.payload = std::move(dest);
    }
};
}  // namespace detail

template <>
struct BufferParser<String> : detail::StringBinaryParser<String> {
    explicit BufferParser(String& val) : StringBinaryParser(val) {}
};

template <>
struct BufferFormatter<String> : detail::StringBinaryFormatter<String> {
    explicit BufferFormatter(const String& val) : StringBinaryFormatter(val) {}
};

template <>
struct BufferParser<LongString> : detail::StringBinaryParser<LongString> {
    explicit BufferParser(LongString& val) : StringBinaryParser(val) {}
};

template <>
struct BufferFormatter<LongString> : detail::StringBinaryFormatter<LongString> {
    explicit BufferFormatter(const LongString& val) : StringBinaryFormatter(val) {}
};

namespace traits {
// for non scalar types or strong typedefs
template <>
struct Output<std::string> {
    using type = detail::CommonStringBinaryFormatter;
};

template <>
struct Input<std::string> {
    using type = detail::CommonStringBinaryParser;
};
}  // namespace traits

}  // namespace cassandra::io
