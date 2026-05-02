#pragma once

#include <cassandra/detail/result_wrapper.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <cassandra/row.hpp>
#include <userver/logging/log.hpp>

namespace cassandra {

/**
 * @file result_set.hpp
 * @brief Cassandra query result set handling and row iteration
 *
 * This file provides the ResultSet class which represents the result of
 * a query execution. It allows accessing query results as single rows,
 * containers of rows, or individual field values.
 *
 * @see cassandra::Session::Execute for query execution that returns ResultSet
 * objects
 * @see cassandra::Row for accessing individual row data
 */

/**
 * @class ResultSet
 * @brief Represents the result of a Cassandra query execution
 *
 * The ResultSet class provides access to the results returned by a Cassandra
 * query. It supports multiple ways to extract data from the results:
 * - Single row extraction as a custom data type
 * - Extraction into containers (vector, list, etc.)
 * - Row-by-row access through the Front() method
 * - Metadata about affected rows and columns
 *
 * @details
 * ResultSet objects are typically returned by Session::Execute() methods and
 * hold a shared reference to the underlying result data. This allows multiple
 * ResultSet instances to share the same result data efficiently.
 *
 * The ResultSet provides type-safe access to query results through template
 * methods that automatically deserialize Cassandra data types to C++ types.
 *
 * @par Example
 * @code
 * using cassandra::Session;
 * using cassandra::Query;
 * using cassandra::Consistency;
 *
 * // Execute a query that returns a single row
 * Query select_user("SELECT id, name, email FROM users WHERE id = ?");
 * ResultSet result = session.Execute(Consistency::kQuorum, select_user,
 * user_id);
 *
 * if (!result.Empty()) {
 *     // Extract as a struct
 *     struct User {
 *         int id;
 *         std::string name;
 *         std::string email;
 *     };
 *     User user = result.AsSingleRow<User>(io::kRowTag);
 *
 *     // Or extract multiple rows into a container
 *     std::vector<User> all_users =
 * result.AsContainer<std::vector<User>>(io::kRowTag);
 * }
 * @endcode
 *
 * @see cassandra::Session::Execute for query execution
 * @see cassandra::Row for low-level row data access
 * @see cassandra::io::RowTag and cassandra::io::FieldTag for result extraction
 * options
 */
class ResultSet {
public:
    /**
     * @brief Checks if the result set contains no rows
     *
     * @return true if the result set is empty or if the underlying result
     *         wrapper is null, false otherwise
     *
     * @details This method should be called before attempting to access rows
     * to avoid undefined behavior.
     *
     * @par Example
     * @code
     * ResultSet result = session.Execute(Consistency::kQuorum, query, id);
     * if (!result.Empty()) {
     *     // Process results
     *     auto row = result.Front();
     * }
     * @endcode
     */
    bool Empty() const { return !_pimpl || _pimpl->Empty(); }

    /**
     * @brief Gets the number of rows affected by the query
     *
     * @return The number of rows that were affected by an UPDATE, INSERT, or
     *         DELETE query. For SELECT queries, this typically returns 0
     * unless the query modified the result set in some way.
     *
     * @details This is primarily useful for mutation operations (INSERT,
     * UPDATE, DELETE) to determine how many rows were modified.
     *
     * @par Example
     * @code
     * ResultSet result = session.Execute(Consistency::kQuorum, delete_query,
     * id); LOG_INFO() << "Deleted " << result.RowsAffected() << " rows";
     * @endcode
     */
    auto RowsAffected() const { return _pimpl->RowsAffected(); }

    /**
     * @brief Gets the number of columns in each result row
     *
     * @return The number of columns that each row in the result set contains.
     *         This value is consistent across all rows in the result set.
     *
     * @details This indicates the number of columns returned by the query.
     * This is useful for validation or when iterating through row data.
     *
     * @par Example
     * @code
     * ResultSet result = session.Execute(Consistency::kQuorum, query);
     * LOG_DEBUG() << "Result has " << result.ColumnsAffected() << " columns";
     * @endcode
     */
    auto ColumnsAffected() const { return _pimpl->ColumnsAffected(); }

    /**
     * @brief Constructs a ResultSet from a result wrapper
     *
     * @param pimpl A shared pointer to the underlying result wrapper
     *
     * @details This is an internal constructor used by the driver. Client code
     * does not typically construct ResultSet objects directly; they are
     * returned by Session::Execute methods.
     *
     * @internal
     */
    ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl) : _pimpl(pimpl) {}

    /**
     * @brief Extracts the first row as a single value or struct
     *
     * @tparam T The C++ type to deserialize the row into. Can be:
     *           - A simple type like int, std::string for single-column
     * results
     *           - A struct with fields matching the query columns (with
     * io::RowTag)
     *
     * @param tag io::FieldTag to extract a single field from the first column,
     *            or io::RowTag to extract an entire row as a struct
     *
     * @return The deserialized row data as type T
     *
     * @throws cassandra::exceptions::Error if the row cannot be deserialized
     *         to the requested type, or if the result set is empty
     *
     * @details This method is useful when you expect exactly one row in the
     * result set and want to extract it as a C++ value or struct. It calls
     * Front() to get the first row and then deserializes it.
     *
     * @par Example
     * @code
     * // Extract a single column value
     * Query count_query("SELECT COUNT(*) FROM users WHERE active = true");
     * ResultSet result = session.Execute(Consistency::kOne, count_query);
     * int count = result.AsSingleRow<int>(io::kFieldTag);
     *
     * // Extract an entire row as a struct
     * struct User { int id; std::string name; };
     * Query user_query("SELECT id, name FROM users WHERE id = ?");
     * ResultSet result = session.Execute(Consistency::kQuorum, user_query,
     * user_id); User user = result.AsSingleRow<User>(io::kRowTag);
     * @endcode
     *
     * @see Front() for accessing just the first row
     * @see AsContainer() for extracting multiple rows
     */
    template <class T>
    T AsSingleRow(io::FieldTag tag) const {
        return Front().As<T>(tag);
    }

    /**
     * @brief Extracts the first row as a single value or struct (row-level
     * extraction)
     *
     * @tparam T The C++ type to deserialize the row into. This should be a
     * struct with fields matching the query columns.
     *
     * @param tag io::RowTag indicating row-level extraction
     *
     * @return The deserialized row as type T
     *
     * @throws cassandra::exceptions::Error if the row cannot be deserialized
     *         to the requested type, or if the result set is empty
     *
     * @details This is the row-level version of AsSingleRow. It extracts an
     * entire row from the result set and deserializes it into the specified
     * struct type.
     *
     * @par Example
     * @code
     * struct Product {
     *     int id;
     *     std::string name;
     *     double price;
     * };
     *
     * Query query("SELECT id, name, price FROM products WHERE id = ?");
     * ResultSet result = session.Execute(Consistency::kQuorum, query,
     * product_id);
     *
     * if (!result.Empty()) {
     *     Product product = result.AsSingleRow<Product>(io::kRowTag);
     *     LOG_INFO() << "Product: " << product.name << " - $" <<
     * product.price;
     * }
     * @endcode
     *
     * @see AsSingleRow(io::FieldTag) for single field extraction
     * @see AsContainer() for extracting multiple rows
     */
    template <class T>
    T AsSingleRow(io::RowTag tag) const {
        return Front().As<T>(tag);
    }

    /**
     * @brief Extracts all rows into a container of a specific type
     *
     * @tparam Container A container type that supports push_back() and
     * reserve() (e.g., std::vector, std::list)
     *
     * @param tag io::RowTag indicating that each row should be extracted as
     *            a complete row (not just a single field)
     *
     * @return A container filled with deserialized rows. The container type's
     *         value_type T is automatically deduced from the Container
     * parameter.
     *
     * @throws cassandra::exceptions::Error if any row cannot be deserialized
     *         to the container's value_type
     *
     * @details This method iterates through all rows in the result set and
     * deserializes each one into the container's value type. It automatically
     * reserves space in the container for efficiency.
     *
     * @par Example
     * @code
     * struct User {
     *     int id;
     *     std::string name;
     *     std::string email;
     * };
     *
     * Query query("SELECT id, name, email FROM users WHERE active = true");
     * ResultSet result = session.Execute(Consistency::kQuorum, query);
     *
     * // Extract all rows into a vector
     * std::vector<User> users =
     * result.AsContainer<std::vector<User>>(io::kRowTag); LOG_INFO() << "Found
     * " << users.size() << " active users";
     *
     * for (const auto& user : users) {
     *     LOG_DEBUG() << user.name << " <" << user.email << ">";
     * }
     *
     * // Also works with other container types
     * std::list<User> user_list =
     * result.AsContainer<std::list<User>>(io::kRowTag);
     * @endcode
     *
     * @see AsSingleRow() for extracting just the first row
     * @see Row for low-level row data access
     */
    template <class Container>
    Container AsContainer(io::RowTag tag) const {
        auto content = _pimpl->RowsContentView();
        using ElementType = typename Container::value_type;
        Container result;
        result.reserve(content.size());
        for (const auto& row : content) {
            result.push_back(Row(row, _pimpl->ColumnsAffected()).As<ElementType>(tag)
            );
        }
        return result;
    }

    /**
     * @brief Retrieves the first row from the result set
     *
     * @return A Row object representing the first row in the result set
     *
     * @throws Undefined behavior if result set is empty. Always check Empty()
     *         before calling this method.
     *
     * @details Returns a Row object that provides low-level access to the
     * first row's data. The returned Row object is valid only for the lifetime
     * of the ResultSet object.
     *
     * This method is useful for manual row data extraction when you need
     * more control over deserialization.
     *
     * @par Example
     * @code
     * ResultSet result = session.Execute(Consistency::kQuorum, query);
     *
     * if (!result.Empty()) {
     *     Row first_row = result.Front();
     *
     *     // Access individual fields
     *     int id = first_row.As<int>(io::kFieldTag);
     *
     *     // Or extract entire row
     *     struct User { int id; std::string name; };
     *     User user = first_row.As<User>(io::kRowTag);
     * }
     * @endcode
     *
     * @see AsSingleRow() for convenience methods to extract the first row
     * @see Row for detailed row data access methods
     */
    Row Front() const {
        return Row(_pimpl->RowsContentView().front(), _pimpl->ColumnsAffected());
    }

private:
    /**
     * @brief Underlying result wrapper holding the actual query results
     *
     * This is a shared pointer to the internal result wrapper that holds
     * the deserialized data from the Cassandra server response.
     */
    std::shared_ptr<detail::ResultWrapper> _pimpl;
};

}  // namespace cassandra