#pragma once

#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/traits.hpp>
#include <cstring>
#include <variant>

namespace cassandra::io {

struct NotSetTag {};
constexpr NotSetTag kNotSetTag{};

struct Value {
    using SizeType = Int;
    using UnderlyingType = protocol::RawBuffer;
    using Payload = std::variant<NotSetTag, Bytes>;
    Payload payload;
};

namespace detail {

template <typename Derived>
struct ValueFormattingMixin {
    using Mixin = ValueFormattingMixin<Derived>;
    void operator()(Value& value) {
        Bytes payload;
        static_cast<Derived&>(*this)(payload);
        value.payload = std::move(payload);
    }
};

struct ValueFormatter : BufferFormatterBase<Value> {
    using BaseType = BufferFormatterBase<Value>;
    using BaseType::BaseType;
    using SizeType = Value::SizeType;
    using Underlying = Value::UnderlyingType;

    void operator()(protocol::RawBuffer& buffer) {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Bytes>) {
                    WriteBuffer(buffer, arg);
                } else if constexpr (std::is_same_v<T, NotSetTag>) {
                    WriteBuffer<SizeType>(buffer, -2);
                }
            },
            this->value.payload
        );
    }
};

struct NotSetTagFormatter : BufferFormatterBase<NotSetTag> {
    using BaseType = BufferFormatterBase<NotSetTag>;
    using BaseType::BaseType;

    void operator()(Value& buffer) { buffer.payload = this->value; }
};
}  // namespace detail

template <>
struct BufferFormatter<NotSetTag> : detail::NotSetTagFormatter {
    explicit BufferFormatter(const NotSetTag& value)
        : detail::NotSetTagFormatter(value){};
};

template <>
struct BufferFormatter<Value> : detail::ValueFormatter {
    explicit BufferFormatter(const Value& value) : detail::ValueFormatter(value){};
};

}  // namespace cassandra::io
