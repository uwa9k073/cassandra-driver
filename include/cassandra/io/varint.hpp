#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/value.hpp>
#include <cstddef>
#include <userver/utils/strong_typedef.hpp>
#include "cassandra/exception.hpp"
#include "cassandra/io/buffer_io.hpp"
#include "cassandra/io/bytes.hpp"
#include "cassandra/io/protocol/types.hpp"
#include "cassandra/io/traits.hpp"
namespace cassandra::io {
/**
 * A variable length two's complement encoding of a signed integer.
 * examples:
 *    0:   0x00
 *    1:   0x01
 *  127:   0x7f
 *  128: 0x0080
 *  129: 0x0081
 *   -1:   0xff
 * -128:   0x80
 * -129: 0xff7f
 * Note that positive numbers must use a most-significant byte with a value
 * less than 0x80, because a most-significant bit of 1 indicates a negative
 * value. Implementors should pad positive values that have a MSB >= 0x80
 * with a leading 0x00 byte.
 * Notice:
 * This class can't store a variable integer that std::int64_t can't hold,
 * I should provide a real varint later, but for now this is easier to implement
 * and have better performance.
 */

struct Varint : userver::utils::StrongTypedef<Varint, BigInt> {
    using StrongTypedef::StrongTypedef;

    using SizeType = BigInt;

    constexpr static std::size_t kSize = sizeof(SizeType);
};

namespace detail {
struct VarintParser : BufferParserBase<Varint> {
    using BaseType::BaseType;

    void operator()(const Bytes& bytes) {
        protocol::RawBufferView payload = std::get<1>(bytes.payload);
        std::size_t offset = 0;
        this->value = ReadBuffer<ValueType>(payload, offset);
    }

    void operator()(protocol::RawBufferView buffer, std::size_t& offset) {
        protocol::RawBufferView payload = buffer.subspan(offset);

        std::array<std::byte, Varint::kSize> storage;

        const bool negative = (std::to_integer<Byte>(payload.front()) & 0x80) != 0;

        storage.fill(negative ? std::byte{0xFF} : std::byte{0x00});

        std::copy(payload.begin(), payload.end(), storage.end() - payload.size());

        Varint::UnderlyingType be{};
        std::memcpy(&be, storage.data(), Varint::kSize);

        this->value = Varint{
            static_cast<Varint::UnderlyingType>(boost::endian::big_to_native(be))
        };
    }
};
struct VarintFormatter : BufferFormatterBase<Varint>,
                         ValueFormattingMixin<VarintFormatter> {
    using BaseType::BaseType;
    using Mixin::Mixin;

    void operator()(Bytes& bytes) {
        Bytes::UnderlyingType payload;  // std::vector<std::byte>
        WriteBuffer<Varint>(payload, this->value);

        bytes.payload = std::move(payload);
    }

    void operator()(protocol::RawBuffer& buffer) {
        auto be = boost::endian::native_to_big(
            static_cast<BigInt>(this->value.GetUnderlying())
        );

        const auto* data = reinterpret_cast<const std::byte*>(&be);

        std::size_t start = 0;

        while (start < sizeof(be) - 1) {
            auto first = std::to_integer<std::uint8_t>(data[start]);
            auto second = std::to_integer<std::uint8_t>(data[start + 1]);

            bool positive_padding = first == 0x00 && (second & 0x80) == 0;

            bool negative_padding = first == 0xFF && (second & 0x80) != 0;

            if (!positive_padding && !negative_padding) break;

            ++start;
        }

        std::size_t payload_size = sizeof(be) - start;

        std::size_t old_size = buffer.size();

        buffer.resize(old_size + payload_size);

        std::memcpy(buffer.data() + old_size, data + start, payload_size);
    }
};

}  // namespace detail

template <>
struct BufferParser<Varint> : detail::VarintParser {
    using detail::VarintParser::VarintParser;
};

template <>
struct BufferFormatter<Varint> : detail::VarintFormatter {
    using detail::VarintFormatter::VarintFormatter;
};
}  // namespace cassandra::io
