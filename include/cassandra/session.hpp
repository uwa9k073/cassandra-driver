#pragma once
#include <cassandra/batch_query.hpp>
#include <cassandra/detail/query_parameters.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <memory>
#include <optional>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/zstring_view.hpp>

namespace cassandra {
namespace detail {
class SessionImpl;
using SessionImplPtr = std::unique_ptr<SessionImpl>;
}  // namespace detail

/**
 * @file session.hpp
 * @brief Cassandra session management and query execution
 *
 * This file provides the Session class which is the main interface for
 * executing queries against a Cassandra database. It manages connections,
 * connection pooling, and provides both single-query and batch-query
 * execution.
 */

/**
 * @class Session
 * @brief Main interface for executing queries against a Cassandra cluster
 *
 * The Session class manages a pool of connections to Cassandra nodes and
 * provides methods for executing queries with various consistency levels and
 * command controls. It supports:
 *
 * - Single query execution with parameterization
 * - Batch query execution
 * - Connection pooling and automatic reconnection
 * - Consistency level specification
 * - Per-query and per-session command controls (timeouts, etc.)
 * - Query preparation and caching
 *
 * @details
 * Session objects are thread-safe and designed to be long-lived. They should
 * typically be created once during application startup and shared across
 * multiple threads.
 *
 * The Session uses a pimpl (Pointer to Implementation) pattern where all
 * actual work is delegated to SessionImpl. This allows for clean separation
 * between the public interface and internal implementation details.
 *
 * Query execution is asynchronous with respect to the server but synchronous
 * from the client's perspective - Execute methods block until a result is
 * received.
 *
 * @par Example
 * @code
 * using cassandra::Session;
 * using cassandra::NodeDescription;
 * using cassandra::SessionSettings;
 * using cassandra::Query;
 * using cassandra::Consistency;
 *
 * // Set up node descriptions
 * std::vector<NodeDescription> nodes = {
 *     NodeDescription{
 *         .use_ssl = false,
 *         .use_compression = true,
 *         .allow_all = true,
 *         .contact_point = cassandra::ContactPoint{"localhost"},
 *         .port = cassandra::Port{9042}
 *     }
 * };
 *
 * // Configure session
 * SessionSettings settings{
 *     .keyspace_name = "my_keyspace"
 * };
 *
 * // Create session (normally done through the Cassandra component)
 * auto session = std::make_shared<Session>(
 *     nodes,
 *     resolver,  // DNS resolver
 *     task_processor,  // userver task processor
 *     settings,
 *     metrics_storage
 * );
 *
 * // Execute queries
 * Query q("SELECT * FROM users WHERE id = ?");
 * ResultSet result = session->Execute(Consistency::kQuorum, q, user_id);
 *
 * if (!result.Empty()) {
 *     auto user = result.AsSingleRow<UserStruct>(io::kRowTag);
 * }
 * @endcode
 *
 * @see Query for query representation
 * @see ResultSet for result handling
 * @see BatchQueryStore for batch operations
 * @see CommandControl for execution options
 * @see Consistency for consistency level options
 */
class Session {
public:
    /**
     * @brief Constructs a new Cassandra session
     *
     * @param node_description A vector of node descriptions specifying
     * Cassandra cluster nodes to connect to
     * @param resolver A pointer to the DNS resolver used for hostname
     * resolution (can be nullptr if using IP addresses)
     * @param task_processor A reference to the userver task processor for
     * handling asynchronous operations
     * @param session_settings Configuration settings for the session including
     *                        keyspace, pool settings, and connection settings
     * @param metrics_storage An optional metrics storage for collecting
     * statistics about query execution
     *
     * @details
     * The Session constructor initializes all necessary internal structures,
     * connection pools, and prepares to connect to the specified Cassandra
     * nodes. Actual connections are established lazily when the first query is
     * executed.
     *
     * The session takes ownership of the internal implementation and manages
     * its lifecycle automatically.
     *
     * @see SessionSettings for configuration options
     * @see NodeDescription for node configuration
     */
    Session(
        std::span<NodeDescription> node_description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& task_processor,
        SessionSettings session_settings,
        userver::utils::statistics::MetricsStoragePtr metrics_storage
    );

    /**
     * @brief Destructor
     *
     * Closes all connections and cleans up resources. The destructor is
     * responsible for properly shutting down the internal SessionImpl.
     */
    ~Session();

    /**
     * @brief Executes a query with string view and arbitrary parameters
     *
     * @tparam Args Parameter types to bind to the query placeholders
     *
     * @param level The consistency level for the query execution
     * @param query A string view containing the CQL query (must remain valid
     *              until this method returns)
     * @param args Variable arguments to bind to query placeholders in order
     *
     * @return A ResultSet containing the query results
     *
     * @throws cassandra::exceptions::SessionError if the session cannot be
     * established
     * @throws cassandra::exceptions::ConnectionError if connection fails
     * @throws cassandra::exceptions::FrameError or subclasses for Cassandra
     * protocol errors
     *
     * @details
     * This template method accepts a query as a string view along with any
     * number of parameters. The parameters are serialized according to their
     * types and sent to the Cassandra server.
     *
     * This overload is useful for simple queries where you want to pass the
     * query string inline along with parameters.
     *
     * @par Example
     * @code
     * // Query with parameters
     * ResultSet result = session->Execute(
     *     Consistency::kQuorum,
     *     "SELECT * FROM users WHERE id = ? AND active = ?",
     *     user_id,
     *     true
     * );
     * @endcode
     */
    template <typename... Args>
    ResultSet Execute(
        Consistency level, userver::utils::zstring_view query, Args&&... args
    );

    /**
     * @brief Executes a Query object with consistency level
     *
     * @tparam Args Parameter types to bind to query placeholders
     *
     * @param level The consistency level for query execution
     * @param query The Query object containing the CQL statement
     * @param args Variable arguments to bind to query placeholders
     *
     * @return A ResultSet containing the query results
     *
     * @throws cassandra::exceptions::SessionError on session establishment
     * failure
     * @throws cassandra::exceptions::ConnectionError on connection failure
     * @throws cassandra::exceptions::FrameError or subclasses for Cassandra
     * protocol errors
     *
     * @details
     * This is the standard way to execute queries. It accepts a Query object
     * and optional parameters to bind to the query placeholders.
     *
     * No CommandControl is specified, so the session's default timeouts apply.
     *
     * @par Example
     * @code
     * Query q("SELECT * FROM profiles WHERE user_id = ?");
     * ResultSet result = session->Execute(Consistency::kOne, q, user_id);
     * @endcode
     */
    template <typename... Args>
    ResultSet Execute(Consistency level, const Query& query, Args&&... args) {
        return Execute(level, std::nullopt, query, std::forward<Args>(args)...);
    }

    /**
     * @brief Executes a Query with consistency level and command control
     *
     * @tparam Args Parameter types to bind to query placeholders
     *
     * @param level The consistency level for query execution
     * @param statement_cmd_ctl Command control specifying timeouts and
     * prepared statement handling options
     * @param query The Query object containing the CQL statement
     * @param args Variable arguments to bind to query placeholders
     *
     * @return A ResultSet containing the query results
     *
     * @throws cassandra::exceptions::SessionError on session establishment
     * failure
     * @throws cassandra::exceptions::ConnectionError on connection failure
     * @throws cassandra::exceptions::FrameError or subclasses for Cassandra
     * protocol errors
     *
     * @details
     * This overload allows specifying per-query command controls such as
     * individual timeouts and prepared statement options. These settings
     * override the session defaults for this specific query.
     *
     * @par Example
     * @code
     * CommandControl cmd_ctl(
     *     std::chrono::milliseconds{5000},  // network timeout
     *     std::chrono::milliseconds{3000},  // statement timeout
     *     CommandControl::PreparedStatementsOptionOverride::kEnabled
     * );
     *
     * Query q("SELECT * FROM events WHERE id = ?");
     * ResultSet result = session->Execute(
     *     Consistency::kQuorum,
     *     cmd_ctl,
     *     q,
     *     event_id
     * );
     * @endcode
     */
    template <typename... Args>
    ResultSet Execute(
        Consistency level,
        CommandControl statement_cmd_ctl,
        const Query& query,
        Args&&... args
    ) {
        LOG_DEBUG(
            "EXECUTE WITH CTL: {}",
            static_cast<int>(statement_cmd_ctl.prepared_statements_enabled)
        );
        return Execute(
            level,
            OptionalCommandControl{statement_cmd_ctl},
            query,
            std::forward<Args>(args)...
        );
    }

    /**
     * @brief Executes multiple queries as a single batch operation
     *
     * @param store A BatchQueryStore containing multiple queries with
     * parameters
     *
     * @return A ResultSet with batch execution results
     *
     * @throws cassandra::exceptions::SessionError on session establishment
     * failure
     * @throws cassandra::exceptions::ConnectionError on connection failure
     * @throws cassandra::exceptions::FrameError or subclasses for Cassandra
     * protocol errors
     *
     * @details
     * Batch execution groups multiple queries together and sends them to
     * Cassandra as a single batch. This is more efficient than executing
     * queries individually when you have many related operations.
     *
     * The batch's consistency level is determined by the level specified
     * when constructing the BatchQueryStore.
     *
     * Batch operations use the session's default command controls.
     *
     * @par Example
     * @code
     * BatchQueryStore batch(Consistency::kQuorum);
     * batch.AddQuery("INSERT INTO users (id, name) VALUES (?, ?)", user1_id,
     * "Alice"); batch.AddQuery("INSERT INTO users (id, name) VALUES (?, ?)",
     * user2_id, "Bob"); batch.AddQuery("INSERT INTO users (id, name) VALUES
     * (?, ?)", user3_id, "Charlie");
     *
     * ResultSet result = session->BatchExecute(batch);
     * @endcode
     *
     * @see BatchQueryStore for building batch queries
     */
    ResultSet BatchExecute(const BatchQueryStore& store);

    /**
     * @brief Executes a batch with custom command control
     *
     * @param store A BatchQueryStore containing multiple queries with
     * parameters
     * @param statement_cmd_ctl Command control specifying timeouts and
     * prepared statement handling for this batch operation
     *
     * @return A ResultSet with batch execution results
     *
     * @throws cassandra::exceptions::SessionError on session establishment
     * failure
     * @throws cassandra::exceptions::ConnectionError on connection failure
     * @throws cassandra::exceptions::FrameError or subclasses for Cassandra
     * protocol errors
     *
     * @details
     * This is the same as BatchExecute(const BatchQueryStore&) but allows
     * specifying per-batch command controls to override session defaults.
     *
     * @par Example
     * @code
     * CommandControl cmd_ctl(
     *     std::chrono::milliseconds{10000},  // longer timeout for batch
     *     std::chrono::milliseconds{5000}
     * );
     *
     * BatchQueryStore batch(Consistency::kAll);
     * batch.AddQuery(query1, param1);
     * batch.AddQuery(query2, param2);
     *
     * ResultSet result = session->BatchExecute(batch, cmd_ctl);
     * @endcode
     *
     * @see BatchQueryStore for building batch queries
     * @see CommandControl for timeout and execution options
     */
    ResultSet BatchExecute(
        const BatchQueryStore& store, CommandControl statement_cmd_ctl
    );

    /**
     * @brief Internal query execution method (protected template
     * specialization)
     *
     * @tparam Args Parameter types
     *
     * @param level Consistency level
     * @param statement_cmd_ctl Optional command control override
     * @param query The Query object to execute
     * @param args Parameters to bind
     *
     * @return A ResultSet with execution results
     *
     * @internal
     * This is the main templated Execute implementation that all other Execute
     * overloads eventually call. It serializes the parameters and delegates to
     * DoExecute for the actual protocol-level execution.
     */
    template <typename... Args>
    ResultSet Execute(
        Consistency level,
        OptionalCommandControl statement_cmd_ctl,
        const Query& query,
        const Args&... args
    ) {
        detail::StaticQueryParameters<sizeof...(args)> params;
        params.Write(args...);
        return DoExecute(level, query, QueryParameters{params}, statement_cmd_ctl);
    }

private:
    /// @brief Pointer to the internal SessionImpl implementation
    detail::SessionImplPtr _pimpl;

    /**
     * @brief Performs the actual query execution at the protocol level
     *
     * @param level Consistency level for the query
     * @param query The Query object to execute
     * @param params Serialized query parameters
     * @param statement_cmd_ctl Optional per-query command control
     *
     * @return A ResultSet with execution results
     *
     * @internal
     * This method delegates to the SessionImpl to perform the actual
     * protocol-level query execution.
     */
    ResultSet DoExecute(
        Consistency level,
        const Query& query,
        const QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );
};

}  // namespace cassandra