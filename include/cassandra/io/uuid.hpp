#pragma once

#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/traits.hpp>
#include <cassandra/io/value.hpp>
#include <exception>
#include <iterator>

namespace cassandra::io {
namespace detail {

struct UuidParser : BufferParserBase<boost::uuids::uuid> {
    using BaseType = BufferParserBase<boost::uuids::uuid>;
    using BaseType::BaseType;

    void operator()(const Bytes& buffer) {
        protocol::RawBufferView view =
            std::get<Bytes::UnderlyingType>(buffer.payload);

        auto size = boost::uuids::uuid::static_size();

        if (view.size() != size) {
            throw std::exception();
        }

        std::memcpy(this->value.begin(), view.data(), size);
    }
};

struct UuidFormatter : BufferFormatterBase<boost::uuids::uuid>,
                       ValueFormattingMixin<UuidFormatter> {
    using BaseType = BufferFormatterBase<boost::uuids::uuid>;
    using BaseType::BaseType;

    using Underlying = Bytes::UnderlyingType;

    using Mixin = ValueFormattingMixin<UuidFormatter>;
    using Mixin::operator();

    void operator()(Bytes& buffer) {
        Underlying raw_buffer;

        std::copy(
            reinterpret_cast<const std::byte*>(this->value.begin()),
            reinterpret_cast<const std::byte*>(this->value.end()),
            std::back_inserter(raw_buffer)
        );

        buffer.payload = std::move(raw_buffer);
    }
};
}  // namespace detail

template <>
struct BufferParser<boost::uuids::uuid> : detail::UuidParser {
    explicit BufferParser(boost::uuids::uuid& val) : detail::UuidParser(val) {}
};

template <>
struct BufferFormatter<boost::uuids::uuid> : detail::UuidFormatter {
    explicit BufferFormatter(const boost::uuids::uuid& val)
        : detail::UuidFormatter(val) {}
};

}  // namespace cassandra::io
