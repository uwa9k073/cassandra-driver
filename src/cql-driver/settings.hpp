#pragma once

#include <string>
#include <chrono>
#include <optional>

namespace cql {

/// Authentication types supported by the driver
enum class AuthenticationType { kNone, kPlainText, kSha256 };

/// Compression types supported by the driver
enum class CompressionType { kNone, kLZ4, kSnappy };

/// Connection settings for CQL driver
struct ConnectionSettings {
  /// Size of prepared statements LRU cache
  std::size_t max_prepared_cache_size{1000};

  /// Whether to use prepared statements by default
  bool use_prepared_statements{true};

  /// Connection timeout
  std::chrono::milliseconds connect_timeout{1000};

  /// Socket read/write timeout
  std::chrono::milliseconds socket_timeout{1000};

  /// Authentication settings
  AuthenticationType auth_type{AuthenticationType::kNone};
  std::string username;
  std::string password;

  /// Compression settings
  CompressionType compression{CompressionType::kNone};

  /// Default consistency level for queries (1-5)
  /// 1 = ONE, 2 = QUORUM, 3 = LOCAL_ONE, 4 = LOCAL_QUORUM, 5 = ALL
  uint16_t default_consistency{1};

  /// Maximum number of requests that can be in-flight
  std::size_t max_pending_requests{1024};

  /// Protocol version to use (3 or 4)
  uint8_t protocol_version{4};

  /// CQL version to announce in STARTUP message
  std::string cql_version{"3.0.0"};

  /// Initial keyspace to use after connection
  std::optional<std::string> keyspace;

  bool operator==(const ConnectionSettings& other) const {
    return max_prepared_cache_size == other.max_prepared_cache_size &&
           use_prepared_statements == other.use_prepared_statements &&
           connect_timeout == other.connect_timeout &&
           socket_timeout == other.socket_timeout &&
           auth_type == other.auth_type && username == other.username &&
           password == other.password && compression == other.compression &&
           default_consistency == other.default_consistency &&
           max_pending_requests == other.max_pending_requests &&
           protocol_version == other.protocol_version &&
           cql_version == other.cql_version && keyspace == other.keyspace;
  }

  bool operator!=(const ConnectionSettings& other) const {
    return !(*this == other);
  }
};
}  // namespace cql
