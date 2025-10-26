#pragma once

#include <cassandra/cassandra_fwd.hpp>

namespace cassandra {
enum class Consistency {
  kAny,
  kOne,
  kTwo,
  kThree,
  kQuorum,
  kAll,
  kLocalQuorum,
  kEachQuorum,
  kSerial,
  kLocalSerial,
  kLocalOne
};

struct CommandControl {
  /// Overall timeout for a command being executed
  TimeoutDuration network_timeout_ms{};
  /// PostgreSQL server-side timeout
  TimeoutDuration statement_timeout_ms{};

  enum class PreparedStatementsOptionOverride {
    kNoOverride,
    kEnabled,
    kDisabled
  };

  PreparedStatementsOptionOverride prepared_statements_enabled{
      PreparedStatementsOptionOverride::kNoOverride};

  constexpr CommandControl(
      TimeoutDuration network_timeout_ms, TimeoutDuration statement_timeout_ms,
      PreparedStatementsOptionOverride prepared_statements_enabled =
          PreparedStatementsOptionOverride::kNoOverride)
      : network_timeout_ms(network_timeout_ms),
        statement_timeout_ms(statement_timeout_ms),
        prepared_statements_enabled(prepared_statements_enabled) {}

  constexpr CommandControl WithExecuteTimeout(
      TimeoutDuration n) const noexcept {
    return {n, statement_timeout_ms};
  }

  constexpr CommandControl WithStatementTimeout(
      TimeoutDuration s) const noexcept {
    return {network_timeout_ms, s};
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

  /// Limits number of concurrent establishing connections (0 - unlimited)
  std::size_t connecting_limit{kDefaultConnectingLimit};

  bool operator==(const PoolSettings& rhs) const {
    return min_size == rhs.min_size && max_size == rhs.max_size &&
           max_queue_size == rhs.max_queue_size &&
           connecting_limit == rhs.connecting_limit;
  }
};
}  // namespace cassandra
