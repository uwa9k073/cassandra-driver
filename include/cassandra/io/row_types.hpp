#pragma once

#include <boost/current_function.hpp>
#include <boost/pfr.hpp>
#include <tuple>
#include <type_traits>
#include <userver/utils/text_light.hpp>
#include <userver/utils/void_t.hpp>
namespace cassandra::io {

struct RowTag {};
struct FieldTag {};

constexpr RowTag kRowTag{};
constexpr FieldTag kFieldTag{};

namespace traits {

template <bool Value>
using BoolConstant = std::integral_constant<bool, Value>;
template <std::size_t Value>
using SizeConstant = std::integral_constant<std::size_t, Value>;
struct ForDeserializationTag;

#ifdef __clang__
constexpr std::string_view kExpectedPrefix =
    "io::traits::IsInNamespaceImpl(std::string_view) [T = ";
#else
constexpr std::string_view kExpectedPrefix =
    "io::traits::IsInNamespaceImpl(std::string_view) [with T = ";
#endif

template <typename T>
constexpr bool IsInNamespaceImpl(std::string_view nsp) {
    using userver::utils::text::StartsWith;
    constexpr std::string_view fname = BOOST_CURRENT_FUNCTION;
    constexpr auto pos = fname.find(kExpectedPrefix);
    if (pos == std::string_view::npos) {
        return false;
    }
    constexpr std::string_view fname_short{fname.data() + pos, fname.size() - pos};
    static_assert(
        !fname_short.empty(),
        "Your compiler produces an unexpected function pretty name"
    );
    return StartsWith(fname_short.substr(kExpectedPrefix.size()), nsp) &&
           StartsWith(fname_short.substr(kExpectedPrefix.size() + nsp.size()), "::");
}

template <typename T>
constexpr bool IsInNamespace(std::string_view nsp) {
    return IsInNamespaceImpl<std::remove_const_t<std::decay_t<T>>>(nsp);
}

template <typename T>
inline constexpr bool kIsInStdNamespace = IsInNamespace<T>("std");
template <typename T>
inline constexpr bool kIsInBoostNamespace = IsInNamespace<T>("boost");

template <typename T>
constexpr bool DetectIsSuitableRowType() {
    using type = std::remove_cv_t<T>;
    return std::is_class_v<type> && !std::is_empty_v<type> &&
           boost::pfr::is_implicitly_reflectable_v<type, ForDeserializationTag> &&
           !std::is_polymorphic_v<type> && !std::is_union_v<type> &&
           !kIsInStdNamespace<type> && !kIsInBoostNamespace<type>;
};

template <typename T>
struct IsTuple : std::false_type {};
template <typename... T>
struct IsTuple<std::tuple<T...>> : std::true_type {};
template <typename T>
struct IsSuitableRowType : BoolConstant<DetectIsSuitableRowType<T>()> {};
enum class RowCategoryType { kNonRow, kTuple, kAggregate, kIntrusiveIntrospection };
template <RowCategoryType Tag>
using RowCategoryConstant = std::integral_constant<RowCategoryType, Tag>;
template <typename T>
struct RowCategory : std::conditional_t<
                         IsTuple<T>::value,
                         RowCategoryConstant<RowCategoryType::kTuple>,
                         std::conditional_t<
                             IsSuitableRowType<T>::value,
                             RowCategoryConstant<RowCategoryType::kAggregate>,
                             RowCategoryConstant<RowCategoryType::kNonRow>>> {};

template <typename T>
inline constexpr RowCategoryType kRowCategory = RowCategory<T>::value;

template <typename T>
constexpr void AssertIsValidRowType() {
    static_assert(
        kRowCategory<T> != RowCategoryType::kNonRow,
        "Row type must be one of the following: "
        "1. primitive type. "
        "2. std::tuple. "
        "3. Aggregation type. See std::aggregation. "
        "4. Has a Introspect method that makes the std::tuple from your "
        "class/struct. "
        "For more info see `uPg: Typed PostgreSQL results` chapter in docs."
    );
}

template <typename T>
inline constexpr bool kIsRowType = kRowCategory<T> != RowCategoryType::kNonRow;

template <typename T>
inline constexpr bool kIsCompositeType = kIsRowType<T>;

template <typename T>
inline constexpr bool kIsColumnType = kRowCategory<T> == RowCategoryType::kNonRow;

template <typename T, typename Enable = USERVER_NAMESPACE::utils::void_t<>>
struct ExtractionTag {
    using type = FieldTag;
};

template <typename T>
struct ExtractionTag<T, std::enable_if_t<kIsRowType<T>>> {
    using type = RowTag;
};

template <typename T>
inline constexpr typename ExtractionTag<T>::type kExtractionTag{};

}  // namespace traits

template <typename T, traits::RowCategoryType C>
struct RowTypeImpl {
    static_assert(
        traits::kRowCategory<T> != traits::RowCategoryType::kNonRow,
        "This type cannot be used as a row type"
    );
};

template <typename T>
struct RowTypeImpl<T, traits::RowCategoryType::kTuple> {
    using ValueType = T;
    using TupleType = T;
    static constexpr std::size_t size = std::tuple_size<TupleType>::value;
    using IndexSequence = std::make_index_sequence<size>;

    static TupleType& GetTuple(ValueType& v) { return v; }
    static const TupleType& GetTuple(const ValueType& v) { return v; }
};

template <typename T>
struct RowTypeImpl<T, traits::RowCategoryType::kAggregate> {
    using ValueType = T;
    using TupleType =
        decltype(boost::pfr::structure_tie(std::declval<ValueType&>()));
    static constexpr std::size_t size = std::tuple_size<TupleType>::value;

    using IndexSequence = std::make_index_sequence<size>;
    static TupleType GetTuple(ValueType& v) { return boost::pfr::structure_tie(v); }
    static auto GetTuple(const ValueType& value) {
        return boost::pfr::structure_to_tuple(value);
    }
};

template <typename T>
struct RowType : RowTypeImpl<T, traits::kRowCategory<T>> {};

}  // namespace cassandra::io
