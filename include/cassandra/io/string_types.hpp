#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>
#include <cstring>
#include <userver/utils/strong_typedef.hpp>

namespace cassandra::io::detail {

template <typename T>
struct StringBinaryParser : BufferParserBase<T> {
    using BaseType = BufferParserBase<T>;
    using BaseType::BaseType;
    using SizeType = typename T::TagType;
    using Underlying = typename T::UnderlyingType;

    void operator()(std::span<const std::byte> data, size_t& offset) {
        auto size = Read<SizeType>(data, offset);
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
        auto size = Read<SizeType>(data, offset);
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
        auto size = buffer.size();
        this->value.reserve(size);
        this->value.resize(size);
        std::memcpy(
            this->value.data(),
            reinterpret_cast<const char*>(buffer.GetUnderlying().data()),
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
        Write<SizeType>(buffer, size);
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

struct CommonStringBinaryFormatter {
    using SizeType = Short;
    std::string value;
    explicit CommonStringBinaryFormatter(const std::string& val)
        : value(val.data(), val.size()) {}

    void operator()(protocol::RawBuffer& buffer) const {
        auto size = static_cast<SizeType>(value.size());
        Write<SizeType>(buffer, size);
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
        buffer = Bytes{std::move(dest)};
    }
};

template <>
struct BufferParser<String> : StringBinaryParser<String> {
    explicit BufferParser(String& val) : StringBinaryParser(val) {}
};

template <>
struct BufferFormatter<String> : StringBinaryFormatter<String> {
    explicit BufferFormatter(const String& val) : StringBinaryFormatter(val) {}
};

template <>
struct BufferParser<LongString> : StringBinaryParser<LongString> {
    explicit BufferParser(LongString& val) : StringBinaryParser(val) {}
};

template <>
struct BufferFormatter<LongString> : StringBinaryFormatter<LongString> {
    explicit BufferFormatter(const LongString& val) : StringBinaryFormatter(val) {}
};

// for non scalar types or strong typedefs
template <>
struct Output<std::string> {
    using type = CommonStringBinaryFormatter;
};

template <>
struct Input<std::string> {
    using type = CommonStringBinaryParser;
};

}  // namespace cassandra::io::detail
