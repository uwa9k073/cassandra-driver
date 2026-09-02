#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cstddef>
#include <optional>
#include <string>

namespace cassandra {

/**
 * @file options.hpp
 * @brief Configuration options for Cassandra sessions and queries
 *
 * This file provides enums and structures for configuring Cassandra sessions,
 * connection pools, and query execution parameters. It includes consistency
 * levels, command controls (timeouts), and various session/pool settings.
 */

/**
 * @enum InitMode
 * @brief Session initialization mode
 *
 * Determines whether the session should be initialized synchronously or
 * asynchronously during construction.
 */
enum class InitMode {
    kSync,  ///< Initialize connections synchronously during Session
            ///< construction
    kAsync  ///< Initialize connections asynchronously after Session
            ///< construction
};

/**
 * @enum Consistency
 * @brief Cassandra query consistency levels
 *
 * Consistency levels control how many replicas must acknowledge a query
 * before the response is returned to the client. Higher consistency levels
 * provide stronger guarantees but may have higher latency and availability
 * costs.
 *
 * @details
 * The consistency level determines the trade-off between availability,
 * partition tolerance, and consistency (CAP theorem). Different consistency
 * levels are suitable for different use cases:
 *
 * - **Strong Consistency**: kAll, kEachQuorum, kSerial, kLocalSerial
 *   Use for operations requiring maximum data correctness
 *
 * - **Quorum-based**: kQuorum, kLocalQuorum
 *   Default choice balancing consistency and availability
 *
 * - **Weak Consistency**: kOne, kTwo, kThree, kLocalOne, kAny
 *   Use for read-heavy workloads prioritizing availability
 *
 * @note The availability of certain consistency levels depends on the
 * replication factor and cluster topology configuration.
 */
enum class Consistency : io::Short {
    /**
     * No consistency guarantees.
     * @deprecated This level should rarely be used in production.
     */
    kAny = 0x0000,

    /**
     * At least one replica acknowledges the request.
     * Provides minimal consistency but highest availability.
     * Good for non-critical data like counters or metrics.
     */
    kOne = 0x0001,

    /**
     * At least two replicas acknowledge the request.
     * Slightly stronger than kOne but still relatively weak consistency.
     */
    kTwo = 0x0002,

    /**
     * At least three replicas acknowledge the request.
     * Provides moderate consistency and availability.
     */
    kThree = 0x0003,

    /**
     * A majority of replicas must acknowledge the request.
     * For replication factor (RF) = 3, quorum = 2 replicas.
     * For RF = 5, quorum = 3 replicas.
     * Good general-purpose consistency level balancing consistency and
     * availability.
     */
    kQuorum = 0x0004,

    /**
     * All replicas must acknowledge the request.
     * Provides strong consistency but may fail if any replica is unavailable.
     * Use only when maximum consistency is required.
     */
    kAll = 0x0005,

    /**
     * Quorum of replicas in the local data center acknowledge the request.
     * Recommended for multi-data-center deployments when operating locally.
     * Provides quorum consistency without waiting for remote data centers.
     */
    kLocalQuorum = 0x0006,

    /**
     * Quorum of replicas in each data center acknowledge the request.
     * Requires quorum acknowledgement from all data centers.
     * Use for data that must be consistent across all data centers.
     */
    kEachQuorum = 0x0007,

    /**
     * Lightweight transaction serialization level.
     * Used with conditional updates (IF conditions).
     * Provides serializable isolation for compare-and-set operations.
     */
    kSerial = 0x0008,

    /**
     * Lightweight transaction serialization level local to the data center.
     * Like kSerial but operates only within the local data center.
     * Use for conditional updates in multi-data-center deployments.
     */
    kLocalSerial = 0x0009,

    /**
     * One replica in the local data center acknowledges the request.
     * Similar to kOne but ensures at least one local replica.
     * Use when local consistency is sufficient.
     */
    kLocalOne = 0x000A
};

/**
 * @struct CommandControl
 * @brief Execution parameters controlling timeout and prepared statement
 * behavior
 *
 * CommandControl provides per-query control over execution timeouts and
 * whether prepared statements should be used. These settings override session
 * defaults for individual queries or batches.
 *
 * @details
 * The CommandControl structure allows fine-grained control over:
 * - **Network timeout**: Maximum time to wait for a response from Cassandra
 * - **Statement timeout**: Server-side timeout for query execution
 * - **Prepared statements**: Whether to use prepared statements for this query
 *
 * All fields are optional. A zero/default value means no override (use session
 * defaults).
 *
 * @par Example
 * @code
 * using cassandra::CommandControl;
 * using cassandra::Consistency;
 *
 * // Long-running query with extended timeouts
 * CommandControl ctl(
 *     std::chrono::milliseconds{30000},  // 30 second network timeout
 *     std::chrono::milliseconds{25000}   // 25 second server timeout
 * );
 *
 * // Query with prepared statements disabled
 * CommandControl no_prep(
 *     std::chrono::milliseconds{5000},
 *     std::chrono::milliseconds{3000},
 *     CommandControl::PreparedStatementsOptionOverride::kDisabled
 * );
 *
 * // Chained timeout modification
 * CommandControl with_longer_timeout = ctl.WithExecuteTimeout(
 *     std::chrono::milliseconds{60000}
 * );
 * @endcode
 *
 * @see cassandra::Session::Execute for using CommandControl with queries
 */
struct CommandControl {
    /**
     * @enum PreparedStatementsOptionOverride
     * @brief Options for controlling prepared statement usage
     */
    enum class PreparedStatementsOptionOverride {
        kNoOverride,  ///< Use session default prepared statement setting
        kEnabled,     ///< Force use of prepared statements for this command
        kDisabled     ///< Force ad-hoc queries, do not use prepared statements
    };

    /**
     * @brief Overall timeout for the command (network operation timeout)
     *
     * Specifies the maximum time to wait for a response from the Cassandra
     * server. A zero value means use the session default timeout.
     *
     * @details
     * This is the wall-clock timeout for the entire operation including:
     * - Connection establishment (if needed)
     * - Query transmission
     * - Server processing
     * - Response reception
     */
    TimeoutDuration network_timeout_ms{};

    /**
     * @brief Server-side timeout for query execution
     *
     * Specifies the maximum time Cassandra will spend executing the query
     * before timing out on the server side. A zero value means use the session
     * default or Cassandra's default.
     *
     * @details
     * This timeout is sent to Cassandra and enforced server-side. If a query
     * takes longer than this on the server, Cassandra will abort it.
     * Note: This is a hint; Cassandra may not honor it for all query types.
     */
    TimeoutDuration statement_timeout_ms{};

    /**
     * @brief Override for prepared statement usage
     *
     * Controls whether prepared statements should be used for this command.
     * A kNoOverride value means use the session's default prepared statement
     * setting.
     */
    PreparedStatementsOptionOverride prepared_statements_enabled{
        PreparedStatementsOptionOverride::kNoOverride
    };

    /**
     * @brief Constructs a CommandControl with specified timeouts and options
     *
     * @param network_timeout_ms Network operation timeout
     * @param statement_timeout_ms Server-side statement timeout
     * @param prepared_statements_enabled Prepared statement override option
     *
     * @details
     * All parameters are optional with default values.
     */
    constexpr CommandControl(
        TimeoutDuration network_timeout_ms,
        TimeoutDuration statement_timeout_ms,
        PreparedStatementsOptionOverride prepared_statements_enabled =
            PreparedStatementsOptionOverride::kNoOverride
    )
        : network_timeout_ms(network_timeout_ms),
          statement_timeout_ms(statement_timeout_ms),
          prepared_statements_enabled(prepared_statements_enabled) {}

    /**
     * @brief Creates a new CommandControl with updated network timeout
     *
     * @param n The new network timeout
     *
     * @return A new CommandControl with the network timeout updated and other
     *         settings copied from this instance
     *
     * @details
     * This is a convenience method for creating modified copies of
     * CommandControl. It follows the builder pattern with immutable semantics.
     *
     * @par Example
     * @code
     * CommandControl ctl(5s, 3s);
     * CommandControl longer_ctl = ctl.WithExecuteTimeout(10s);
     * // ctl remains unchanged, longer_ctl has new timeout
     * @endcode
     */
    constexpr CommandControl WithExecuteTimeout(TimeoutDuration n) const noexcept {
        return {n, statement_timeout_ms};
    }

    /**
     * @brief Creates a new CommandControl with updated statement timeout
     *
     * @param s The new statement timeout
     *
     * @return A new CommandControl with the statement timeout updated and
     * other settings copied from this instance
     *
     * @details
     * This is a convenience method for creating modified copies of
     * CommandControl.
     *
     * @par Example
     * @code
     * CommandControl ctl(5s, 3s);
     * CommandControl longer_stmt_ctl = ctl.WithStatementTimeout(4s);
     * // ctl remains unchanged
     * @endcode
     */
    constexpr CommandControl WithStatementTimeout(TimeoutDuration s) const noexcept {
        return {network_timeout_ms, s};
    }

    /**
     * @brief Equality comparison operator
     *
     * @param rhs The CommandControl to compare with
     *
     * @return true if all fields are equal, false otherwise
     */
    bool operator==(const CommandControl& rhs) const {
        return network_timeout_ms == rhs.network_timeout_ms &&
               statement_timeout_ms == rhs.statement_timeout_ms &&
               prepared_statements_enabled == rhs.prepared_statements_enabled;
    }

    /**
     * @brief Inequality comparison operator
     *
     * @param rhs The CommandControl to compare with
     *
     * @return true if any field differs, false if all equal
     */
    bool operator!=(const CommandControl& rhs) const { return !(*this == rhs); }
};

/**
 * @typedef OptionalCommandControl
 * @brief An optional CommandControl that may not be set
 *
 * Used to distinguish between "no override specified" (std::nullopt) and
 * "use these controls" (contains a CommandControl).
 *
 * @see cassandra::CommandControl for timeout and execution control options
 */
using OptionalCommandControl = std::optional<CommandControl>;

/**
 * @brief Default minimum number of connections in the pool
 *
 * The pool will maintain at least this many connections to the cluster.
 * These connections are pre-established during session initialization.
 */
inline constexpr std::size_t kDefaultPoolMinSize = 4;

/**
 * @brief Default maximum replication lag threshold
 *
 * Used by the driver to detect replicas that are lagging behind and
 * potentially exclude them from query routing.
 */
inline constexpr auto kDefaultMaxReplicationLag = std::chrono::seconds{60};

/**
 * @brief Default maximum number of connections in the pool
 *
 * The pool will not create more than this many connections.
 * When the pool reaches this limit, new requests wait in the queue.
 */
inline constexpr std::size_t kDefaultPoolMaxSize = 15;

/**
 * @brief Default maximum queue size for clients waiting for connections
 *
 * When all connections in the pool are busy, new requests are queued.
 * This constant limits how many requests can wait for a connection.
 * Requests exceeding this limit will receive an error.
 */
inline constexpr std::size_t kDefaultPoolMaxQueueSize = 200;

/**
 * @brief Default limit for concurrent connection establishments
 *
 * Limits how many connections can be established simultaneously.
 * 0 means unlimited (no limit on concurrent connection attempts).
 * Use this to prevent connection storms during cluster issues.
 */
inline constexpr std::size_t kDefaultConnectingLimit = 0;

/**
 * @struct PoolSettings
 * @brief Configuration for connection pooling behavior
 *
 * PoolSettings controls how the session manages its pool of connections
 * to Cassandra nodes. Proper pool settings are critical for performance
 * and resource utilization.
 *
 * @details
 * The connection pool works as follows:
 * 1. Session creates min_size connections during initialization
 * 2. Pool expands up to max_size connections as needed
 * 3. When pool is full, requests queue up to max_queue_size capacity
 * 4. When queue is full, new requests receive an error
 * 5. Idle connections are removed to avoid resource waste
 *
 * The connecting_limit controls how aggressively the pool expands by
 * limiting simultaneous connection establishment.
 *
 * @par Example
 * @code
 * using cassandra::PoolSettings;
 *
 * // Aggressive pool configuration for high-throughput scenarios
 * PoolSettings aggressive{
 *     .min_size = 10,
 *     .max_size = 50,
 *     .max_queue_size = 500,
 *     .connecting_limit = 5  // Establish up to 5 connections concurrently
 * };
 *
 * // Conservative pool for low-throughput or memory-constrained scenarios
 * PoolSettings conservative{
 *     .min_size = 1,
 *     .max_size = 5,
 *     .max_queue_size = 50,
 *     .connecting_limit = 1
 * };
 * @endcode
 *
 * @see cassandra::SessionSettings for using PoolSettings in session
 * configuration
 */
struct PoolSettings final {
    /**
     * @brief Number of connections created initially
     *
     * Default: kDefaultPoolMinSize (4)
     *
     * The session will establish this many connections to each node
     * during session initialization. This helps avoid connection
     * establishment latency for the first queries.
     *
     * Increase this for high-throughput scenarios to pre-establish
     * capacity. Decrease for low-traffic or memory-constrained systems.
     */
    std::size_t min_size{kDefaultPoolMinSize};

    /**
     * @brief Maximum number of connections that can be created
     *
     * Default: kDefaultPoolMaxSize (15)
     *
     * The pool will expand to this size when needed to handle increased
     * load. Each connection consumes resources (memory, file descriptors),
     * so set this based on your workload and resource constraints.
     *
     * For typical applications, 5-15 per node is reasonable.
     * For very high-throughput applications, use 20-30+.
     */
    std::size_t max_size{kDefaultPoolMaxSize};

    /**
     * @brief Maximum number of requests queued waiting for a connection
     *
     * Default: kDefaultPoolMaxQueueSize (200)
     *
     * When all connections are busy, new requests queue here.
     * If the queue fills, new requests immediately fail with an error.
     * This prevents unbounded memory growth during load spikes.
     *
     * Increase this if you expect temporary load spikes.
     * Decrease it to fail fast during overload conditions.
     */
    std::size_t max_queue_size{kDefaultPoolMaxQueueSize};

    /**
     * @brief Limit for concurrent connection establishments
     *
     * Default: kDefaultConnectingLimit (0 = unlimited)
     *
     * Limits how many connections can be established simultaneously.
     * This prevents connection storms during cluster issues or recovery.
     *
     * Set to 0 for no limit (fast recovery, potential storm).
     * Set to 1-5 for gradual connection establishment (smoother load).
     * Typical range: 0-5 depending on recovery speed preferences.
     */
    std::size_t connecting_limit{kDefaultConnectingLimit};

    std::size_t prepared_statement_cache_ways;
    std::size_t prepared_statement_cache_way_size;
    bool prepared_statement_cache_enabled;

    /**
     * @brief Equality comparison operator
     *
     * @param rhs The PoolSettings to compare with
     *
     * @return true if all settings match, false otherwise
     */
    bool operator==(const PoolSettings& rhs) const {
        return min_size == rhs.min_size && max_size == rhs.max_size &&
               max_queue_size == rhs.max_queue_size &&
               connecting_limit == rhs.connecting_limit;
    }
};

/**
 * @struct ConnectionSettings
 * @brief Per-connection configuration for Cassandra connections
 *
 * ConnectionSettings provides options that apply to individual connections
 * within the connection pool.
 *
 * @details
 * These settings control per-connection behavior such as:
 * - Maximum time-to-live for connections
 * - Error handling and reconnection thresholds
 *
 * @par Example
 * @code
 * using cassandra::ConnectionSettings;
 *
 * ConnectionSettings settings{
 *     .max_ttl = std::chrono::seconds{3600},  // 1 hour connection lifetime
 *     .recent_errors_threshold = 3            // Reconnect after 3 errors
 * };
 * @endcode
 *
 * @see cassandra::SessionSettings for using cassandra::ConnectionSettings in
 * session configuration
 */
struct ConnectionSettings {
    /**
     * @brief Maximum time-to-live for individual connections
     *
     * Default: std::nullopt (no limit)
     *
     * If set, connections will be automatically recycled after this duration.
     * This is useful to periodically refresh connections or prevent long-lived
     * connections with stale state.
     *
     * Set to a value like 1 hour to refresh connections periodically.
     * Leave as nullopt for indefinite connection lifetime.
     */
    std::optional<std::chrono::seconds> max_ttl;

    /**
     * @brief Threshold for recent errors before reconnecting
     *
     * Default: 2
     *
     * If a connection experiences this many recent errors, it will be
     * automatically closed and a new connection established.
     * This helps recover from transient connection issues.
     *
     * Lower values (1-2) provide faster recovery from transient issues.
     * Higher values (3-5) reduce unnecessary reconnections.
     */
    std::size_t recent_errors_threshold = 2;
};

/**
 * @struct SessionSettings
 * @brief Complete configuration for a Cassandra session
 *
 * SessionSettings bundles all the configuration needed to create and run
 * a Cassandra session. It includes keyspace selection, pool configuration,
 * and per-connection settings.
 *
 * @details
 * SessionSettings is typically created once and used for all sessions
 * accessing the same keyspace and cluster. It can be customized for
 * different workload characteristics.
 *
 * @par Example
 * @code
 * using cassandra::SessionSettings;
 * using cassandra::PoolSettings;
 * using cassandra::ConnectionSettings;
 *
 * // Standard settings for a production keyspace
 * SessionSettings prod_settings{
 *     .keyspace_name = "production",
 *     .pool_settings = PoolSettings{
 *         .min_size = 5,
 *         .max_size = 20,
 *         .max_queue_size = 200,
 *         .connecting_limit = 3
 *     },
 *     .connection_settings = ConnectionSettings{
 *         .max_ttl = std::chrono::seconds{3600},
 *         .recent_errors_threshold = 2
 *     }
 * };
 *
 * // Lightweight settings for a cache/temporary keyspace
 * SessionSettings cache_settings{
 *     .keyspace_name = "cache_data",
 *     .pool_settings = PoolSettings{
 *         .min_size = 1,
 *         .max_size = 5
 *     }
 * };
 * @endcode
 *
 * @see cassandra::Session for creating sessions with these settings
 * @see cassandra::PoolSettings for connection pool tuning
 * @see cassandra::ConnectionSettings for per-connection options
 */
struct SessionSettings {
    /**
     * @brief The Cassandra keyspace to use for this session
     *
     * All queries executed through this session will use this keyspace
     * by default. This is required and must be a valid keyspace name
     * that exists on the cluster.
     *
     * @par Example
     * @code
     * SessionSettings settings{
     *     .keyspace_name = "my_application_keyspace"
     * };
     * @endcode
     */
    std::string keyspace_name;
    std::string load_balancing_policy;
    /**
     * @brief Connection pool settings
     *
     * Configures the pool of connections used by this session.
     * Default settings work well for most applications.
     *
     * @see cassandra::PoolSettings for detailed configuration options
     */
    PoolSettings pool_settings{};

    /**
     * @brief Per-connection settings
     *
     * Configures behavior of individual connections within the pool.
     *
     * @see cassandra::ConnectionSettings for available options
     */
    ConnectionSettings connection_settings{};
};
}  // namespace cassandra
