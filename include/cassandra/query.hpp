#pragma once

#include <cassandra/io/cassandra_types.hpp>
#include <userver/storages/query.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/zstring_view.hpp>
#include <utility>

namespace cassandra {

/**
 * @file query.hpp
 * @brief Cassandra Query Language (CQL) query representation and management
 *
 * This file provides the Query class which wraps CQL query strings
 * and manages their lifecycle. Queries can be constructed from various
 * string representations and are used with Session::Execute methods.
 */

/**
 * @class Query
 * @brief Represents a Cassandra Query Language (CQL) query string
 *
 * The Query class is a lightweight wrapper around a CQL query string that manages
 * the lifetime of the query data. It supports multiple construction methods
 * and is designed to be copyable and movable.
 *
 * @details
 * The Query class provides a unified interface for working with query strings
 * in different forms:
 * - Raw string views (for compile-time known queries)
 * - String literals (for userver's compile-time strings)
 * - Dynamic strings (allocated at runtime)
 * - io::LongString objects (internal type)
 *
 * Queries are used to execute CQL statements through a Session object.
 * The Query object itself does not execute anything; it merely represents
 * the query string to be sent to the Cassandra server.
 *
 * @example
 * @code
 * using cassandra::Query;
 * using cassandra::Consistency;
 *
 * // From string view (compile-time)
 * Query q1("SELECT * FROM users WHERE id = ?");
 *
 * // From std::string (runtime)
 * std::string query_str = BuildQueryDynamically();
 * Query q2(query_str);
 *
 * // From StringLiteral (userver compile-time)
 * Query q3(userver::utils::StringLiteral{"SELECT * FROM profiles"});
 *
 * // Execute the query through a session
 * ResultSet result = session.Execute(Consistency::kQuorum, q1, user_id);
 * @endcode
 *
 * @see Session::Execute for how to use queries with query execution
 * @see ResultSet for handling query results
 */
class Query {
public:
    /**
     * @brief Default constructor creating an empty query
     *
     * Creates a Query object with no statement. Typically used as a placeholder
     * or before assignment.
     */
    Query() = default;

    /**
     * @brief Destructor
     *
     * Releases any resources held by the query (handled automatically by
     * io::LongString).
     */
    ~Query() = default;

    /**
     * @brief Copy constructor - creates a copy of the query
     *
     * Creates an independent copy of the query statement. Both the original
     * and the copy remain valid and can be used independently.
     */
    Query(const Query& other) = default;

    /**
     * @brief Move constructor - transfers ownership of query data
     *
     * Efficiently transfers ownership of the query data from the source
     * to this object. The source object is left in a valid but unspecified state.
     */
    Query(Query&& other) = default;

    /**
     * @brief Copy assignment operator
     *
     * Replaces the current query with a copy of the source query.
     *
     * @return Reference to this Query object
     */
    Query& operator=(const Query& other) = default;

    /**
     * @brief Move assignment operator
     *
     * Replaces the current query by taking ownership of the source query data.
     *
     * @return Reference to this Query object
     */
    Query& operator=(Query&& other) = default;

    /**
     * @brief Constructs a Query from a string view (constexpr)
     *
     * @param statement A view of the CQL query string
     *
     * @details This constructor is constexpr and suitable for compile-time
     * known query strings. The string view must remain valid during the
     * Query construction.
     *
     * @example
     * @code
     * constexpr std::string_view query = "SELECT * FROM table";
     * Query q(query);
     * @endcode
     */
    constexpr Query(std::string_view statement)
        : data_{io::LongString{statement.data(), statement.size()}} {}

    /**
     * @brief Constructs a Query from a userver StringLiteral (constexpr)
     *
     * @param statement A compile-time string literal
     *
     * @details StringLiterals are guaranteed to be compile-time known strings,
     * making this constructor suitable for embedding queries at compile time.
     * This is the recommended way to define queries that are known at compile time.
     *
     * @example
     * @code
     * Query q(userver::utils::StringLiteral{"SELECT * FROM users WHERE id = ?"});
     * @endcode
     */
    constexpr Query(userver::utils::StringLiteral statement)
        : data_{io::LongString{statement.data(), statement.size()}} {}

    /**
     * @brief Constructs a Query from a C-style string
     *
     * @param statement A null-terminated C-style string
     *
     * @details The string is copied internally, so the original string
     * can be safely destroyed after construction. This constructor is useful
     * for working with legacy C code or when you have a raw pointer to a
     * null-terminated string.
     *
     * @example
     * @code
     * const char* cql = "SELECT * FROM events";
     * Query q(cql);
     * @endcode
     */
    Query(const char* statement) : Query(std::string{statement}) {}

    /**
     * @brief Constructs a Query from an std::string (takes ownership)
     *
     * @param statement An std::string containing the CQL query
     *
     * @details The string is moved internally when possible, making this
     * constructor efficient for dynamically constructed queries. The original
     * std::string is moved into the Query and should not be used afterwards.
     *
     * @example
     * @code
     * std::string dynamic_query = "SELECT * FROM table WHERE id = ?";
     * Query q(dynamic_query);
     * @endcode
     */
    Query(std::string statement) : data_{io::LongString{std::move(statement)}} {}

    /**
     * @brief Constructs a Query from an rvalue io::LongString
     *
     * @param statement An rvalue reference to an io::LongString
     *
     * @details This is an internal constructor for use with pre-constructed
     * io::LongString objects. Generally not used directly by client code.
     */
    Query(io::LongString&& statement) : data_{std::move(statement)} {}

    /**
     * @brief Constructs a Query from a const io::LongString reference
     *
     * @param statement A reference to an io::LongString
     *
     * @details This is an internal constructor that copies the io::LongString.
     * Generally not used directly by client code.
     */
    Query(const io::LongString& statement) : data_{statement} {}

    /**
     * @brief Retrieves the underlying CQL statement
     *
     * @return A copy of the underlying io::LongString containing the CQL query
     *
     * @details Returns the internal query string representation. This is
     * typically used internally by the driver for query preparation and execution.
     * Client code rarely needs to call this method directly.
     *
     * @example
     * @code
     * Query q("SELECT * FROM users");
     * auto statement = q.GetStatement();
     * // statement now contains the CQL string
     * @endcode
     */
    io::LongString GetStatement() const { return data_; }

private:
    /**
     * @brief Internal storage of the CQL query string
     *
     * Holds the actual query text in io::LongString format.
     */
    io::LongString data_;
};

}  // namespace cassandra