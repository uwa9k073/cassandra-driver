#pragma once

#include <string>
#include <chrono>

namespace cql {

/// @brief Class representing the current state of a CQL connection
class ConnectionState {
 public:
  /// The possible states a connection can be in
  enum class Status {
    /// Connection has not been initialized
    NOT_CONNECTED,
    /// Attempting to connect
    CONNECTING,
    /// Connected and ready for queries
    CONNECTED,
    /// Connection startup in progress
    AUTHENTICATING,
    /// Connection is being closed
    CLOSING,
    /// Connection has been closed
    CLOSED,
    /// Connection encountered an error
    ERROR
  };

  /// Create connection state with status and optional error
  ConnectionState(Status status, std::string error = {}) noexcept
      : status_(status), error_(std::move(error)) {}

  /// Get the current status
  Status GetStatus() const noexcept { return status_; }

  /// Get error message if any
  const std::string& GetError() const noexcept { return error_; }

  /// Get time since last state change
  std::chrono::system_clock::time_point GetLastStateChangeTime()
      const noexcept {
    return last_state_change_;
  }

  /// Check if connection is in an error state
  bool HasError() const noexcept { return !error_.empty(); }

  /// Convert status to string
  static const std::string StatusToString(Status status) noexcept;

 private:
  Status status_;
  std::string error_;
  std::chrono::system_clock::time_point last_state_change_ =
      std::chrono::system_clock::now();
};
}  // namespace cql
