#pragma once

#include <cassandra/detail/query_parameters.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <string_view>
#include <userver/storages/query.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/zstring_view.hpp>
#include <utility>

namespace cassandra {

/**
 * @file batch_query.hpp
 * @brief Batch query support for executing multiple queries atomically
 *
 * This file provides classes for building and executing batches of queries
 * against a Cassandra cluster. Batch operations allow atomic execution of
 * multiple queries, which is more efficient than executing them individually.
 */

/**
 * @class BatchQuery
 * @brief Represents a single query within a batch with its parameters
 *
 * The BatchQuery class wraps a query string and its bound parameters. It's
 * used as a building block for BatchQueryStore objects that are executed
 * as atomic batches against Cassandra.
 *
 * @details
 * BatchQuery objects are created with a query and optional parameters.
 * The parameters are stored in a QueryParameters object and managed
 * through a shared pointer to ensure proper lifetime management.
 *
 * BatchQuery objects are typically not used directly by client code;
 * instead, they are created implicitly through BatchQueryStore::AddQuery()
 * methods and managed internally by the BatchQueryStore.
 *
 * @example
 * @code
 * using cassandra::BatchQuery;
 *
 * // Typically created through BatchQueryStore:
 * cassandra::BatchQueryStore batch(Consistency::kQuorum);
 * batch.AddQuery("INSERT INTO users (id, name) VALUES (?, ?)", 1, "Alice");
 * batch.AddQuery("INSERT INTO users (id, name) VALUES (?, ?)", 2, "Bob");
 *
 * // Or directly (less common):
 * BatchQuery bq("INSERT INTO events (id, timestamp) VALUES (?, ?)", event_id, now);
 * @endcode
 *
 * @see BatchQueryStore for batch construction and execution
 * @see Session::BatchExecute for executing batch queries
 */
class BatchQuery {
public:
    /**
     * @brief Constructs a BatchQuery from a Query object and parameters
     *
     * @tparam Args Parameter types to bind to query placeholders
     *
     * @param query The Query object containing the CQL statement
     * @param args Query parameters to bind to ? placeholders in order
     *
     * @details
     * Creates a BatchQuery by storing the query and serializing the parameters.
     * Parameters are stored in a DynamicQueryParameters object and a shared
     * pointer is kept to manage their lifetime.
     *
     * @example
     * @code
     * cassandra::Query q("INSERT INTO users (id, name) VALUES (?, ?)");
     * cassandra::BatchQuery bq(q, 123, std::string("John"));
     * @endcode
     */
    template <typename... Args>
    BatchQuery(const Query& query, Args&&... args) : _query{query} {
        detail::DynamicQueryParameters params;
        params.Write(args...);
        _params = QueryParameters{params};
        _params_holder = params.ParamHolder();
    }

    /**
     * @brief Constructs a BatchQuery from a string view and parameters
     *
     * @tparam Args Parameter types to bind to query placeholders
     *
     * @param query A string view containing the CQL statement
     * @param args Query parameters to bind to ? placeholders in order
     *
     * @details
     * Creates a BatchQuery from a string view. The string view is converted
     * to a Query object internally.
     *
     * @example
     * @code
     * cassandra::BatchQuery bq(
     *     std::string_view("UPDATE stats SET count = count + 1 WHERE id = ?"),
     *     user_id
     * );
     * @endcode
     */
    template <typename... Args>
    BatchQuery(std::string_view query, Args&&... args) : _query{std::move(query)} {
        detail::DynamicQueryParameters params;
        params.Write(args...);
        _params = QueryParameters{params};
        _params_holder = params.ParamHolder();
    }

    /**
     * @brief Constructs a BatchQuery from a StringLiteral and parameters
     *
     * @tparam Args Parameter types to bind to query placeholders
     *
     * @param query A userver StringLiteral containing the CQL statement
     * @param args Query parameters to bind to ? placeholders in order
     *
     * @details
     * Creates a BatchQuery from a compile-time string literal. This is
     * the preferred method for queries known at compile time.
     *
     * @example
     * @code
     * cassandra::BatchQuery bq(
     *     userver::utils::StringLiteral{"INSERT INTO log (id, msg) VALUES (?, ?)"},
     *     log_id,
     *     "Error occurred"
     * );
     * @endcode
     */
    template <typename... Args>
    BatchQuery(userver::utils::StringLiteral query, Args&&... args)
        : _query{std::move(query)} {
        detail::DynamicQueryParameters params;
        params.Write(args...);
        _params = QueryParameters{params};
        _params_holder = params.ParamHolder();
    }

    /**
     * @brief Gets the Query object for this batch query
     *
     * @return A const reference to the Query object
     *
     * @details Returns the CQL query string wrapped in a Query object.
     *
     * @example
     * @code
     * BatchQuery bq(query, param1, param2);
     * const Query& q = bq.GetQuery();
     * @endcode
     */
    const Query& GetQuery() const { return _query; }

    /**
     * @brief Gets the query parameters for this batch query
     *
     * @return A const reference to the QueryParameters object
     *
     * @details Returns the bound parameters that will be sent to Cassandra.
     *
     * @example
     * @code
     * BatchQuery bq(query, param1, param2);
     * const QueryParameters& params = bq.GetParams();
     * @endcode
     */
    const QueryParameters& GetParams() const { return _params; }

private:
    /// @brief The CQL query for this batch query
    Query _query;

    /// @brief The bound query parameters
    QueryParameters _params;

    /// @brief Shared pointer holding the parameter data for proper lifetime management
    std::shared_ptr<std::vector<io::Bytes>> _params_holder;
};

/**
 * @struct BatchStatement
 * @brief Internal representation of a single statement in a batch request
 *
 * @details
 * BatchStatement represents a single query statement that will be sent to
 * Cassandra as part of a batch. According to the Cassandra protocol, each
 * statement in a batch can be represented either as:
 * - A query string (Kind::kString)
 * - A prepared statement ID (Kind::kId)
 *
 * BatchStatement objects are typically created automatically by the driver
 * when preparing batch requests. Client code rarely needs to work with them
 * directly.
 *
 * @internal
 */
struct BatchStatement {
    /**
     * @enum Kind
     * @brief Type of batch statement representation
     *
     * @var Kind::kString Query is represented as a full CQL string
     *                   (not pre-prepared)
     * @var Kind::kId Query is represented as a prepared statement ID,
     *                which must have been prepared on this connection
     */
    enum class Kind : io::Byte { kString = 0, kId = 1 };

    /// @brief The type of query representation (string or ID)
    Kind kind;

    /// @brief The query representation - either CQL string or prepared statement ID
    std::variant<io::LongString, io::ShortBytes> query;

    /// @brief The parameters to bind to this statement
    QueryParameters params;

    /**
     * @brief Constructs a BatchStatement from a CQL string and parameters
     *
     * @param query The CQL query string
     * @param params The query parameters
     *
     * @internal
     */
    BatchStatement(const io::LongString& query, const QueryParameters& params)
        : kind(Kind::kString), query(query), params(params) {}

    /**
     * @brief Constructs a BatchStatement from a prepared statement ID and parameters
     *
     * @param id The prepared statement ID
     * @param params The query parameters
     *
     * @internal
     */
    BatchStatement(const io::ShortBytes& id, const QueryParameters& params)
        : kind(Kind::kId), query(id), params(params) {}
};

/**
 * @class BatchQueryStore
 * @brief Builder and container for batch query execution
 *
 * The BatchQueryStore class collects multiple queries and their parameters,
 * and manages their execution as a single atomic batch against Cassandra.
 *
 * @details
 * BatchQueryStore uses a builder pattern to accumulate queries:
 * 1. Create a BatchQueryStore with a consistency level
 * 2. Add queries using AddQuery() methods (returns *this for chaining)
 * 3. Execute the batch through Session::BatchExecute()
 *
 * All queries in a batch are executed atomically - either all succeed or
 * all fail. This provides transactional semantics for related operations.
 *
 * The consistency level applies to all queries in the batch.
 *
 * @example
 * @code
 * using cassandra::BatchQueryStore;
 * using cassandra::Consistency;
 *
 * // Create a batch with quorum consistency
 * BatchQueryStore batch(Consistency::kQuorum);
 *
 * // Add queries using chaining
 * batch.AddQuery("INSERT INTO users (id, name) VALUES (?, ?)", 1, "Alice")
 *      .AddQuery("INSERT INTO users (id, name) VALUES (?, ?)", 2, "Bob")
 *      .AddQuery("UPDATE user_count SET total = total + 2");
 *
 * // Execute the batch
 * ResultSet result = session->BatchExecute(batch);
 * @endcode
 *
 * @see Session::BatchExecute for executing batch queries
 * @see BatchQuery for individual queries in the batch
 */
class BatchQueryStore {
public:
    /**
     * @brief Constructs a new BatchQueryStore with a consistency level
     *
     * @param level The consistency level for all queries in this batch
     *             (e.g., Consistency::kQuorum, Consistency::kLocalOne)
     *
     * @details
     * Creates an empty batch query store. Use AddQuery() to add queries.
     * All queries will be executed with the specified consistency level.
     *
     * @example
     * @code
     * BatchQueryStore batch(Consistency::kLocalQuorum);
     * @endcode
     */
    explicit BatchQueryStore(Consistency level) : _consistency{level} {}

    /**
     * @brief Adds a query with parameters to the batch
     *
     * @tparam Args Parameter types for the query
     *
     * @param query The Query object containing the CQL statement
     * @param args Query parameters to bind to ? placeholders
     *
     * @return A reference to this BatchQueryStore for method chaining
     *
     * @details
     * Adds a new query to the batch. The query and parameters are stored
     * as a BatchQuery object. This method returns *this to allow chaining
     * multiple AddQuery calls.
     *
     * @example
     * @code
     * Query q1("INSERT INTO users (id, name) VALUES (?, ?)");
     * Query q2("INSERT INTO users (id, email) VALUES (?, ?)");
     * batch.AddQuery(q1, 1, "Alice")
     *      .AddQuery(q2, 1, "alice@example.com");
     * @endcode
     */
    template <typename... Args>
    [[maybe_unused]] BatchQueryStore& AddQuery(const Query& query, Args&&... args) {
        _queries.emplace_back(query, std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Adds a query with parameters to the batch (StringLiteral version)
     *
     * @tparam Args Parameter types for the query
     *
     * @param query A userver StringLiteral containing the CQL statement
     * @param args Query parameters to bind to ? placeholders
     *
     * @return A reference to this BatchQueryStore for method chaining
     *
     * @details
     * Adds a query using a compile-time string literal. This is the preferred
     * method for queries that are known at compile time. Supports method chaining.
     *
     * @example
     * @code
     * batch.AddQuery(
     *     userver::utils::StringLiteral{"INSERT INTO events (id, msg) VALUES (?, ?)"},
     *     event_id,
     *     "Event occurred"
     * );
     * @endcode
     */
    template <typename... Args>
    [[maybe_unused]] BatchQueryStore& AddQuery(
        userver::utils::StringLiteral query, Args&&... args
    ) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Adds a query with parameters to the batch (string_view version)
     *
     * @tparam Args Parameter types for the query
     *
     * @param query A string_view containing the CQL statement
     * @param args Query parameters to bind to ? placeholders
     *
     * @return A reference to this BatchQueryStore for method chaining
     *
     * @details
     * Adds a query from a string view. Useful for dynamically constructed queries.
     * Supports method chaining.
     *
     * @example
     * @code
     * std::string query_str = BuildQuery();
     * batch.AddQuery(std::string_view{query_str}, param1, param2);
     * @endcode
     */
    template <typename... Args>
    [[maybe_unused]] BatchQueryStore& AddQuery(
        std::string_view query, Args&&... args
    ) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Gets the consistency level for this batch
     *
     * @return The consistency level used for all queries in the batch
     *
     * @details
     * Returns the consistency level that was specified when constructing
     * this BatchQueryStore.
     *
     * @example
     * @code
     * BatchQueryStore batch(Consistency::kQuorum);
     * assert(batch.ConsistencyLevel() == Consistency::kQuorum);
     * @endcode
     */
    Consistency ConsistencyLevel() const { return _consistency; }

    /**
     * @brief Gets all queries in the batch
     *
     * @return A span of the stored BatchQuery objects
     *
     * @details
     * Returns a span (non-owning view) of all queries that have been added
     * to this batch. This is typically used internally by the driver when
     * preparing to send the batch to Cassandra.
     *
     * @example
     * @code
     * BatchQueryStore batch(Consistency::kQuorum);
     * batch.AddQuery(q1, p1).AddQuery(q2, p2).AddQuery(q3, p3);
     * auto queries = batch.Queries();
     * std::cout << "Batch contains " << queries.size() << " queries" << std::endl;
     * @endcode
     */
    std::span<const BatchQuery> Queries() const { return _queries; }

private:
    /// @brief The consistency level for all queries in the batch
    Consistency _consistency;

    /// @brief The collection of queries in the batch
    std::vector<BatchQuery> _queries;
};

}  // namespace cassandra