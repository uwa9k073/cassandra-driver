#pragma once

#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <type_traits>
#include <userver/compiler/demangle.hpp>
#include <userver/logging/log.hpp>

namespace cassandra {

/**
 * @file row.hpp
 * @brief Row data extraction and type conversion for Cassandra results
 *
 * This file provides the Row class and supporting utilities for extracting
 * and converting data from individual rows in Cassandra query results.
 * It handles deserialization of Cassandra data types to C++ types.
 */

/**
 * @class Row
 * @brief Represents a single row from a Cassandra query result
 *
 * The Row class provides type-safe access to the columns of a row returned
 * from a Cassandra query. It supports conversion of individual columns to
 * C++ types and extraction of entire rows into struct types.
 *
 * @details
 * A Row object wraps a buffer view of the row data along with column count.
 * The buffer contains the serialized column values in Cassandra wire format.
 * The Row provides methods to deserialize these values to C++ types.
 *
 * The Row class supports two main conversion modes:
 * 1. **Field Mode**: Extract a single column value (using io::FieldTag)
 * 2. **Row Mode**: Extract all columns into a struct (using io::RowTag)
 *
 * The Row object does not own the underlying buffer - it's a view into
 * the result set's buffer. Therefore, the Row is only valid for the
 * lifetime of the ResultSet that created it.
 *
 * @par Example
 * @code
 * using cassandra::Row;
 * using cassandra::io::FieldTag;
 * using cassandra::io::RowTag;
 *
 * // Row obtained from ResultSet::Front()
 * Row row = result_set.Front();
 *
 * // Extract single column as int (first column only)
 * int id = row.As<int>(FieldTag);
 *
 * // Extract entire row into a struct
 * struct User {
 *     int id;
 *     std::string name;
 *     std::string email;
 * };
 * User user = row.As<User>(RowTag);
 * @endcode
 *
 * @see ResultSet::Front for obtaining Row objects
 * @see ResultSet::AsSingleRow for convenience single-row extraction
 */
class Row {
public:
    /**
     * @brief Constructs a Row from a buffer view and column count
     *
     * @param buffer A BytesBufferView containing the serialized column data
     * @param columns_count The number of columns in this row
     *
     * @details
     * This constructor is typically called internally by the driver when
     * deserializing query results. Client code rarely constructs Row objects
     * directly. The buffer view must remain valid for the lifetime of the
     * Row object.
     *
     * @internal This is primarily for internal driver use.
     */
    Row(io::protocol::BytesBufferView buffer, int columns_count)
        : _row_content(std::move(buffer)), _columns_count(columns_count) {}

    /**
     * @brief Retrieves the underlying buffer view for this row
     *
     * @return A BytesBufferView containing the raw serialized column data
     *
     * @details
     * Returns the raw buffer containing all serialized columns. This is useful
     * for advanced use cases where you need direct access to the wire-format
     * data. Typically, you should use the As() methods for type-safe
     * conversion instead.
     *
     * @par Example
     * @code
     * Row row = result_set.Front();
     * auto buffer = row.GetBufferView();
     * // buffer contains raw Cassandra wire-format data for all columns
     * @endcode
     */
    io::protocol::BytesBufferView GetBufferView() const { return _row_content; }

    /**
     * @brief Converts the row to an arbitrary type using default conversion
     *
     * @tparam T The target C++ type to deserialize to
     *
     * @return The converted value of type T
     *
     * @throws cassandra::exceptions::Error if the conversion fails
     *
     * @details
     * This is a convenience overload that uses the default tag type
     * (io::kFieldTag). It's primarily useful for extracting single column
     * values. For multi-column conversions, use As() with io::kRowTag.
     *
     * @par Example
     * @code
     * int value = row.As<int>();  // Extracts first column as int
     * @endcode
     */
    template <class T>
    T As() {
        return As<T>(io::kFieldTag);
    }

    /**
     * @brief Converts a single column to an arbitrary type
     *
     * @tparam T The target C++ type
     *
     * @param tag The FieldTag indicating single-field extraction mode
     *
     * @return The converted column value of type T
     *
     * @throws cassandra::exceptions::Error if deserialization fails or if
     *         the column type is incompatible with T
     *
     * @details
     * This method deserializes the first column (index 0) of the row to
     * the specified type. The column value is read from the underlying
     * buffer using a BufferReader specialized for the target type.
     *
     * This is useful for queries that return a single column or when you
     * only care about the first column of each row.
     *
     * @par Example
     * @code
     * // Query returns only one column
     * ResultSet result = session.Execute(Consistency::kQuorum, "SELECT
     * COUNT(*) FROM table"); Row row = result.Front(); int count =
     * row.As<int>(io::kFieldTag);
     * @endcode
     */
    template <class T>
    T As(io::FieldTag tag) const {
        T val;
        To(val, tag);
        return val;
    }

    /**
     * @brief Converts the entire row to a struct type
     *
     * @tparam T The target struct type with a specialized io::RowType
     *
     * @param tag The RowTag indicating row-level extraction mode
     *
     * @return The converted struct value of type T
     *
     * @throws cassandra::exceptions::Error if deserialization fails or if
     *         the row structure doesn't match the target type
     *
     * @details
     * This method deserializes all columns in the row into the specified
     * struct type. The target type must have a specialization of io::RowType
     * defined that describes how to extract columns into the struct.
     *
     * The struct's member variables are populated from the row's columns
     * in order. If the row has fewer columns than the struct expects, or
     * if there's a type mismatch, an exception is thrown.
     *
     * @par Example
     * @code
     * struct Product {
     *     int id;
     *     std::string name;
     *     double price;
     * };
     * // Assuming io::RowType<Product> is properly specialized
     *
     * ResultSet result = session.Execute(
     *     Consistency::kQuorum,
     *     "SELECT id, name, price FROM products WHERE id = ?",
     *     product_id
     * );
     * Row row = result.Front();
     * Product product = row.As<Product>(io::kRowTag);
     * @endcode
     */
    template <class T>
    T As(io::RowTag tag) const {
        T val;
        To(val, tag);
        return val;
    }

    /**
     * @brief Gets the number of columns in this row
     *
     * @return The number of columns in the row
     *
     * @details
     * Returns the column count that was specified when the Row was
     * constructed. All rows from the same query result have the same column
     * count.
     *
     * @par Example
     * @code
     * Row row = result_set.Front();
     * LOG_INFO() << "Row has " << row.Size() << " columns";
     * @endcode
     */
    size_t Size() const { return _columns_count; }

private:
    /// @brief Buffer containing the raw serialized column data
    io::protocol::BytesBufferView _row_content;

    /// @brief Number of columns in this row
    int _columns_count;

    /**
     * @brief Internal conversion for single field extraction
     *
     * @tparam T The target type
     *
     * @param val Reference to the value to populate
     * @param tag The FieldTag indicator
     *
     * @internal This method performs the actual deserialization of the first
     * column into the target type.
     */
    template <typename T>
    void To(T&& val, io::FieldTag) const {
        using ValueType = std::decay_t<T>;

        val = io::BufferReader<io::Bytes>{_row_content.front()}.Read<ValueType>();
    }

    /**
     * @brief Internal conversion for row-level extraction
     *
     * @tparam T The target struct type
     *
     * @param val Reference to the struct to populate
     * @param tag The RowTag indicator
     *
     * @internal This method performs the actual deserialization of all columns
     * into the target struct type.
     */
    template <typename T>
    void To(T&& val, io::RowTag) const;
};

/**
 * @struct RowDataExtractorBase
 * @brief Base template for extracting row data into multiple values or tuples
 *
 * @tparam IndexTuple An index_sequence for accessing tuple elements
 * @tparam T Types of the data elements to extract
 *
 * @internal
 * This is an internal template utility for converting rows into
 * tuples or multiple separate variables. It provides methods to extract
 * all columns from a row into individual variables or into a std::tuple.
 *
 * @details
 * RowDataExtractorBase provides static methods for extracting data from rows:
 * - ExtractValues: Extracts columns into multiple individual variables
 * - ExtractTuple: Extracts columns into a std::tuple
 *
 * The template is specialized on index sequences to ensure compile-time
 * correctness of tuple element access.
 */
template <typename IndexTuple, typename... T>
struct RowDataExtractorBase;

/**
 * @internal
 * Specialization of RowDataExtractorBase for concrete index sequences
 */
template <std::size_t... Indexes, typename... T>
struct RowDataExtractorBase<std::index_sequence<Indexes...>, T...> {
    /**
     * @brief Extracts row columns into multiple individual variables
     *
     * @param row The Row to extract from
     * @param val Parameter pack of references to variables to populate
     *
     * @internal
     * This method iterates through the row's columns and deserializes each
     * one into the corresponding variable in the parameter pack.
     */
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

    /**
     * @brief Extracts row columns into an lvalue tuple reference
     *
     * @param row The Row to extract from
     * @param val Reference to the tuple to populate
     *
     * @internal
     * This method iterates through the row's columns and deserializes each
     * one into the corresponding tuple element.
     */
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

    /**
     * @brief Extracts row columns into an rvalue tuple reference
     *
     * @param row The Row to extract from
     * @param val Rvalue reference to the tuple to populate
     *
     * @internal
     * This overload handles temporary tuples, allowing move semantics.
     */
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

/**
 * @struct RowDataExtractor
 * @brief Public interface for row data extraction using RowDataExtractorBase
 *
 * @tparam T Types of the data elements to extract
 *
 * @internal
 * This template inherits from RowDataExtractorBase, passing an automatically
 * generated index sequence. It's used to extract row columns into tuples or
 * multiple variables with proper compile-time indexing.
 */
template <typename... T>
struct RowDataExtractor : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {
};

/**
 * @struct TupleDataExtractor
 * @brief Specialization of RowDataExtractor for std::tuple types
 *
 * @tparam T A std::tuple type to extract
 *
 * @internal
 * This template automatically extracts the element types from a std::tuple
 * and uses RowDataExtractorBase to perform the actual extraction. This allows
 * clean extraction of rows into tuple types.
 */
template <typename T>
struct TupleDataExtractor;

/**
 * @internal
 * Specialization of TupleDataExtractor for concrete tuple types
 */
template <typename... T>
struct TupleDataExtractor<std::tuple<T...>>
    : RowDataExtractorBase<std::index_sequence_for<T...>, T...> {};

/**
 * @brief Internal row-to-type conversion implementation
 *
 * @details
 * This is a template specialization of Row::To for row-level conversion.
 * It handles the conversion of an entire row into a struct type.
 *
 * @internal
 * This implementation:
 * 1. Determines the struct's tuple representation using io::RowType
 * 2. Validates that the row has enough columns for all struct members
 * 3. Logs a warning if the row has more columns than the struct (unused
 * columns)
 * 4. Extracts all columns into the struct using TupleDataExtractor
 */
template <typename T>
void Row::To(T&& val, io::RowTag) const {
    using ValueType = std::decay_t<T>;

    using RowType = io::RowType<ValueType>;
    using TupleType = typename RowType::TupleType;
    constexpr size_t tuple_size = RowType::size;

    if (tuple_size > Size()) {
        throw ::cassandra::exceptions::Error(fmt::format(
            "Row size ({}) is less than the number of data members in C++ "
            "user "
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