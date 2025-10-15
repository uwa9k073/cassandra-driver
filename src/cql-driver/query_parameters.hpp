#pragma once

#include <cstdint>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace cql {

/// @brief Class representing parameters for a CQL query
class QueryParameters {
 public:
  /// Query consistency level
  enum class Consistency {
    ANY,
    ONE,
    TWO,
    THREE,
    QUORUM,
    ALL,
    LOCAL_QUORUM,
    EACH_QUORUM,
    SERIAL,
    LOCAL_SERIAL,
    LOCAL_ONE
  };

  /// Query serial consistency level for conditional updates
  enum class SerialConsistency { SERIAL, LOCAL_SERIAL };

  /// Default constructor with LOCAL_ONE consistency
  QueryParameters() : consistency_(Consistency::LOCAL_ONE) {}

  /// Set consistency level
  QueryParameters& SetConsistency(Consistency level) {
    consistency_ = level;
    return *this;
  }

  /// Set serial consistency level for conditional updates
  QueryParameters& SetSerialConsistency(SerialConsistency level) {
    serial_consistency_ = level;
    return *this;
  }

  /// Set query timeout
  QueryParameters& SetTimeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
    return *this;
  }

  /// Set page size for paging
  QueryParameters& SetPageSize(int32_t size) {
    page_size_ = size;
    return *this;
  }

  /// Set paging state for continuation
  QueryParameters& SetPagingState(std::vector<std::uint8_t> state) {
    paging_state_ = std::move(state);
    return *this;
  }

  /// Set timestamp for the query
  QueryParameters& SetTimestamp(int64_t timestamp) {
    timestamp_ = timestamp;
    return *this;
  }

  /// Set keyspace for the query
  QueryParameters& SetKeyspace(std::string keyspace) {
    keyspace_ = std::move(keyspace);
    return *this;
  }

  /// Get consistency level
  Consistency GetConsistency() const { return consistency_; }

  /// Get serial consistency level
  const std::optional<SerialConsistency>& GetSerialConsistency() const {
    return serial_consistency_;
  }

  /// Get query timeout
  const std::optional<std::chrono::milliseconds>& GetTimeout() const {
    return timeout_;
  }

  /// Get page size
  const std::optional<int32_t>& GetPageSize() const { return page_size_; }

  /// Get paging state
  const std::optional<std::vector<std::uint8_t>>& GetPagingState() const {
    return paging_state_;
  }

  /// Get timestamp
  const std::optional<int64_t>& GetTimestamp() const { return timestamp_; }

  /// Get keyspace
  const std::optional<std::string>& GetKeyspace() const { return keyspace_; }

 private:
  Consistency consistency_;
  std::optional<SerialConsistency> serial_consistency_;
  std::optional<std::chrono::milliseconds> timeout_;
  std::optional<int32_t> page_size_;
  std::optional<std::vector<std::uint8_t>> paging_state_;
  std::optional<int64_t> timestamp_;
  std::optional<std::string> keyspace_;
};
}  // namespace cql
