#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace cassandra {

enum class InitMode { kSync, kAsync };

enum class Consistency : io::Short {
    kAny = 0x0000,
    kOne = 0x0001,
    kTwo = 0x0002,
    kThree = 0x0003,
    kQuorum = 0x0004,
    kAll = 0x0005,
    kLocalQuorum = 0x0006,
    kEachQuorum = 0x0007,
    kSerial = 0x0008,
    kLocalSerial = 0x0009,
    kLocalOne = 0x000A
};

struct CommandControl {
    /// Overall timeout for a command being executed
    TimeoutDuration network_timeout_ms{};
    /// PostgreSQL server-side timeout
    TimeoutDuration statement_timeout_ms{};

    enum class PreparedStatementsOptionOverride { kNoOverride, kEnabled, kDisabled };

    PreparedStatementsOptionOverride prepared_statements_enabled{
        PreparedStatementsOptionOverride::kNoOverride
    };

    int16_t stream_id;

    constexpr CommandControl(
        TimeoutDuration network_timeout_ms,
        TimeoutDuration statement_timeout_ms,
        PreparedStatementsOptionOverride prepared_statements_enabled =
            PreparedStatementsOptionOverride::kNoOverride,
        int16_t stream_id = 0
    )
        : network_timeout_ms(network_timeout_ms),
          statement_timeout_ms(statement_timeout_ms),
          prepared_statements_enabled(prepared_statements_enabled),
          stream_id(stream_id) {}

    constexpr CommandControl WithExecuteTimeout(TimeoutDuration n) const noexcept {
        return {n, statement_timeout_ms};
    }

    constexpr CommandControl WithStatementTimeout(TimeoutDuration s) const noexcept {
        return {network_timeout_ms, s};
    }

    constexpr CommandControl WithStreamId(std::int16_t k) const noexcept {
        return {
            network_timeout_ms, statement_timeout_ms, prepared_statements_enabled, k
        };
    }

    bool operator==(const CommandControl& rhs) const {
        return network_timeout_ms == rhs.network_timeout_ms &&
               statement_timeout_ms == rhs.statement_timeout_ms &&
               prepared_statements_enabled == rhs.prepared_statements_enabled;
    }

    bool operator!=(const CommandControl& rhs) const { return !(*this == rhs); }
};
/// @brief storages::postgres::CommandControl that may not be set
using OptionalCommandControl = std::optional<CommandControl>;
inline constexpr std::size_t kDefaultPoolMinSize = 4;

/// Default maximum replication lag
inline constexpr auto kDefaultMaxReplicationLag = std::chrono::seconds{60};

/// Default pool connections limit
inline constexpr std::size_t kDefaultPoolMaxSize = 15;

/// Default size of queue for clients waiting for connections
inline constexpr std::size_t kDefaultPoolMaxQueueSize = 200;

/// Default limit for concurrent establishing connections number
inline constexpr std::size_t kDefaultConnectingLimit = 0;

struct PoolSettings final {
    /// Number of connections created initially
    std::size_t min_size{kDefaultPoolMinSize};

    /// Maximum number of created connections
    std::size_t max_size{kDefaultPoolMaxSize};

    /// Maximum number of clients waiting for a connection
    std::size_t max_queue_size{kDefaultPoolMaxQueueSize};

    /// Limits number of concurrent establishing connections (0 -
    /// unlimited)
    std::size_t connecting_limit{kDefaultConnectingLimit};

    bool operator==(const PoolSettings& rhs) const {
        return min_size == rhs.min_size && max_size == rhs.max_size &&
               max_queue_size == rhs.max_queue_size &&
               connecting_limit == rhs.connecting_limit;
    }
};

struct ConnectionSettings {
    std::optional<std::chrono::seconds> max_ttl;
    std::size_t recent_errors_threshold = 2;
};

struct SessionSettings {
    std::string keyspace_name;

    PoolSettings pool_settings{};

    ConnectionSettings connection_settings{};
};
}  // namespace cassandra
