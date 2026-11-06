#pragma once

#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/traits.hpp>
#include <cassandra/io/value.hpp>
#include <cassert>
#include <cstring>
#include <userver/utils/ip.hpp>

namespace cassandra::io {

namespace detail {
template <std::size_t Size>
struct InetParser : BufferParserBase<userver::utils::ip::AddressBase<Size>> {
    using BaseType = BufferParserBase<userver::utils::ip::AddressBase<Size>>;
    using BaseType::BaseType;

    using ValueType = userver::utils::ip::AddressBase<Size>;

    void operator()(const Bytes& buffer) {
        const auto& payload = std::get<1>(buffer.payload);
        if (payload.size() != ValueType::kAddressSize) {
            throw cassandra::exceptions::Error("Invalid IP size");
        }

        typename ValueType::BytesType bytes;

        std::memcpy(bytes.data(), payload.data(), ValueType::kAddressSize);

        this->value = ValueType{bytes};
    }
};

template <std::size_t Size>
struct InetFormatter : BufferFormatterBase<userver::utils::ip::AddressBase<Size>>,
                       ValueFormattingMixin<userver::utils::ip::AddressBase<Size>> {
    using BaseType = BufferFormatterBase<userver::utils::ip::AddressBase<Size>>;
    using Mixin = ValueFormattingMixin<userver::utils::ip::AddressBase<Size>>;

    using BaseType::BaseType;
    using Mixin::operator();

    using ValueType = userver::utils::ip::AddressBase<Size>;

    void operator()(Bytes& buffer) {
        const auto& raw_inet = this->value.GetBytes();
        Bytes::UnderlyingType underlying;
        underlying.resize(raw_inet.size());
        std::memcpy(
            underlying.data(),
            reinterpret_cast<const std::byte*>(raw_inet.data()),
            raw_inet.size()
        );
        buffer.payload = std::move(underlying);
    }
};
}  // namespace detail

template <>
struct BufferParser<InetV4> : detail::InetParser<InetV4::kAddressSize> {
    explicit BufferParser(InetV4& inet)
        : detail::InetParser<InetV4::kAddressSize>(inet) {}
};

template <>
struct BufferParser<InetV6> : detail::InetParser<InetV6::kAddressSize> {
    explicit BufferParser(InetV6& inet)
        : detail::InetParser<InetV6::kAddressSize>(inet) {}
};

template <>
struct BufferFormatter<InetV4> : detail::InetFormatter<InetV4::kAddressSize> {
    explicit BufferFormatter(const InetV4& inet)
        : detail::InetFormatter<InetV4::kAddressSize>(inet) {}
};

template <>
struct BufferFormatter<InetV6> : detail::InetFormatter<InetV6::kAddressSize> {
    explicit BufferFormatter(const InetV6& inet)
        : detail::InetFormatter<InetV6::kAddressSize>(inet) {}
};
}  // namespace cassandra::io
