#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/value.hpp>
#include <type_traits>
#include <variant>

#include <userver/utils/optional_ref.hpp>

#include <boost/optional/optional_fwd.hpp>
#include <cassandra/io/buffer_io.hpp>
namespace cassandra::io {

namespace detail {

template <typename T>
struct GetSetNull {
    inline static bool IsNull(const T&) { return false; }
    inline static void SetNull(T&) {
        // TODO Consider a static_assert here
        // throw TypeCannotBeNull(compiler::GetTypeName<T>());
    }
    inline static void SetDefault(T& value) { value = T{}; }
};

template <typename T>
struct GetSetNull<boost::optional<T>> {
    using ValueType = boost::optional<T>;
    inline static bool IsNull(const ValueType& v) { return !v; }
    inline static void SetNull(ValueType& v) { v = ValueType{}; }
    inline static void SetDefault(ValueType& v) { v.emplace(); }
};

template <typename T>
struct GetSetNull<std::optional<T>> {
    using ValueType = std::optional<T>;
    inline static bool IsNull(const ValueType& v) { return !v; }
    inline static void SetNull(ValueType& v) { v = std::nullopt; }
    inline static void SetDefault(ValueType& v) { v.emplace(); }
};

template <typename T>
struct GetSetNull<userver::utils::OptionalRef<T>> {
    using ValueType = userver::utils::OptionalRef<T>;
    inline static bool IsNull(const ValueType& v) { return !v; }
    inline static void SetNull(ValueType&) {
        static_assert(!sizeof(T), "SetNull not enabled for utils::OptionalRef");
    }
    inline static void SetDefault(ValueType&) {
        static_assert(!sizeof(T), "SetDefault not enabled for utils::OptionalRef");
    }
};

template <template <typename> class Optional, typename T>
struct OptionalValueParser : BufferParserBase<Optional<T>> {
    using BaseType = BufferParserBase<Optional<T>>;
    using ValueParser = typename traits::IO<T>::ParserType;

    using BaseType::BaseType;

    void operator()(const Bytes& buffer) {
        T val;

        std::visit(
            [&](auto&& arg) {
                using P = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<P, Bytes::UnderlyingType>) {
                    val = ReadBuffer<T>(buffer);
                    this->value = std::move(val);
                } else if constexpr (std::is_same_v<P, NullTag>) {
                    GetSetNull<Optional<T>>::SetNull(this->value);
                }
            },
            buffer.payload
        );
    }
};

template <template <typename> class Optional, typename T>
struct OptionalValueFormatter
    : BufferFormatterBase<Optional<T>>,
      ValueFormattingMixin<OptionalValueFormatter<Optional, T>> {
    using Base = BufferFormatterBase<Optional<T>>;
    using Formatter = typename traits::IO<T>::FormatterType;

    using Base::Base;

    using Mixin = ValueFormattingMixin<OptionalValueFormatter<Optional, T>>;

    using Mixin::operator();

    void operator()(Bytes& bytes) {
        if (!this->value) {
            bytes.payload = kNullTag;
            return;
        }

        Formatter{*this->value}(bytes);
    }
};

}  // namespace detail

template <typename T>
struct BufferParser<boost::optional<T>>
    : detail::OptionalValueParser<boost::optional, T> {
    using BaseType = detail::OptionalValueParser<boost::optional, T>;
    using BaseType::BaseType;
};

/// Formatter specialization for boost::optional
template <typename T>
struct BufferFormatter<boost::optional<T>>
    : detail::OptionalValueFormatter<boost::optional, T> {
    using BaseType = detail::OptionalValueFormatter<boost::optional, T>;
    using BaseType::BaseType;
};

/// Parser specialization for std::optional
template <typename T>
struct BufferParser<std::optional<T>>
    : detail::OptionalValueParser<std::optional, T> {
    using BaseType = detail::OptionalValueParser<std::optional, T>;
    using BaseType::BaseType;
};

/// Formatter specialization for std::optional
template <typename T>
struct BufferFormatter<std::optional<T>>
    : detail::OptionalValueFormatter<std::optional, T> {
    using BaseType = detail::OptionalValueFormatter<std::optional, T>;
    using BaseType::BaseType;
};

/// Formatter specialization for utils::OptionalRef
template <typename T>
struct BufferFormatter<userver::utils::OptionalRef<T>>
    : detail::OptionalValueFormatter<userver::utils::OptionalRef, T> {
    using BaseType = detail::OptionalValueFormatter<userver::utils::OptionalRef, T>;
    using BaseType::BaseType;
};
}  // namespace cassandra::io
