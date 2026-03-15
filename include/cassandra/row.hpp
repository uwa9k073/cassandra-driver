#pragma once

#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <type_traits>
#include <userver/compiler/demangle.hpp>
#include <userver/logging/log.hpp>
#include <vector>
#include <cassandra/io/cassandra_types.hpp>

namespace cassandra {
class Row {
public:
    Row(io::protocol::BytesBuffer&& buffer, int columns_count)
        : _row_content(std::move(buffer)), _columns_count(columns_count) {}
    Row(const io::protocol::BytesBuffer& buffer, int columns_count)
        : _row_content(buffer), _columns_count(columns_count) {}

    io::protocol::BytesBufferView GetBufferView() const { return _row_content; }

    template <class T>
    T As() {
        return As<T>(io::kFieldTag);
    }

    template <class T>
    T As(io::FieldTag tag) const {
        T val;
        To(val, tag);
        return val;
    }
    template <class T>
    T As(io::RowTag tag) const {
        T val;
        To(val, tag);
        return val;
    }

    int Size() const { return _columns_count; }

private:
    std::vector<io::Bytes> _row_content;
    int _columns_count;

    template <typename T>
    void To(T&& val, io::FieldTag) const {
        using ValueType = std::decay_t<T>;

        val = io::BufferReader<io::Bytes>{_row_content.front()}.Read<ValueType>();
    }

    template <typename T>
    void To(T&& val, io::RowTag) const;
};

template <typename IndexTuple, typename... T>
struct RowDataExtractorBase;

template <std::size_t... Indexes, typename... T>
struct RowDataExtractorBase<std::index_sequence<Indexes...>, T...> {
    static void ExtractValues(const Row& row, T&&... val) {
        static_assert(sizeof...(Indexes) == sizeof...(T));

        auto buffer_view = row.GetBufferView();
        size_t column_index = 0;
        const auto perform = [&](auto& arg) {
            arg = io::BufferReader<io::Bytes>{buffer_view[column_index++]}
                      .Read<std::decay_t<decltype(arg)>>();
        };

        (perform(std::forward<T>(val)), ...);
    }
    static void ExtractTuple(const Row& row, std::tuple<T...>& val) {
        static_assert(sizeof...(Indexes) == sizeof...(T));

        auto buffer_view = row.GetBufferView();
        size_t column_index = 0;
        const auto perform = [&](auto& arg) {
            arg = io::BufferReader<io::Bytes>{buffer_view[column_index++]}
                      .Read<std::decay_t<decltype(arg)>>();
        };

        (perform(std::get<Indexes>(val)), ...);
    }
    static void ExtractTuple(const Row& row, std::tuple<T...>&& val) {
        static_assert(sizeof...(Indexes) == sizeof...(T));

        auto buffer_view = row.GetBufferView();
        size_t column_index = 0;
        const auto perform = [&](auto& arg) {
            arg = io::BufferReader<io::Bytes>{buffer_view[column_index++]}
                      .Read<std::decay_t<decltype(arg)>>();
        };

        (perform(std::get<Indexes>(val)), ...);
    }
};

template <typename... T>
struct RowDataExtractor : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {
};

template <typename T>
struct TupleDataExtractor;
template <typename... T>
struct TupleDataExtractor<std::tuple<T...>>
    : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {};

template <typename T>
void Row::To(T&& val, io::RowTag) const {
    using ValueType = std::decay_t<T>;

    using RowType = io::RowType<ValueType>;
    using TupleType = typename RowType::TupleType;
    constexpr auto tuple_size = RowType::size;

    if (tuple_size > Size()) {
        throw ::cassandra::exceptions::Error(fmt::format(
            "Row size ({}) is less than the number of data members in C++ user "
            "datatype ({})",
            Size(),
            tuple_size
        ));
    } else if (tuple_size < Size()) {
        LOG_LIMITED_WARNING()
            << "Row size is greater that the number of data members in "
               "C++ user datatype "
            << userver::compiler::GetTypeName<T>();
    }

    TupleDataExtractor<TupleType>::ExtractTuple(
        *this, RowType::GetTuple(std::forward<T>(val))
    );
}
}  // namespace cassandra
