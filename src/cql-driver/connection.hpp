#pragma once

#include <memory>
#include <string>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>

#include "cql-driver/query_parameters.hpp"
#include "dsn.hpp"
#include "prepared_statement.hpp"
#include "query.hpp"
#include "result_set.hpp"
#include "settings.hpp"

namespace cql {

namespace detail {
class ConnectionImpl;
}

/// @brief CQL connection class for executing queries and managing connection
/// state Handles connecting to Cassandra, sending commands, processing results
/// and maintaining connection state. Responsible for all asynchronous
/// operations.
class Connection : public std::enable_shared_from_this<Connection> {
 public:
  Connection(userver::engine::TaskProcessor& bg_task_processor,
             userver::concurrent::BackgroundTaskStorageCore& bg_task_storage,
             uint32_t id, const ConnectionSettings& settings);
  ~Connection();

  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection(Connection&&) = delete;
  Connection& operator=(Connection&&) = delete;

  /// Connect to Cassandra cluster
  void Connect(const Dsn& dsn, userver::engine::Deadline deadline);

  /// Close the connection
  void Close();

  /// Check if connection is established
  bool IsConnected() const;

  /// Check if connection is in a valid state for queries
  bool IsHealthy() const;

  /// Execute a CQL query
  ResultSet Execute(const Query& query, const QueryParameters& params,
                    userver::engine::Deadline deadline);

  /// Prepare a statement for later execution
  std::shared_ptr<PreparedStatement> Prepare(
      const std::string& query, userver::engine::Deadline deadline);

  /// Execute a prepared statement with parameters
  ResultSet ExecutePrepared(const PreparedStatement& stmt,
                            const std::vector<Query::Parameter>& parameters,
                            userver::engine::Deadline deadline);

  /// Get connection statistics
  // const Statistics& GetStatistics() const;

  /// Get connection settings
  const ConnectionSettings& GetSettings() const;

  /// Get the server version
  int GetServerVersion() const;

  /// Get connection state information
  // ConnectionState GetState() const;

 private:
  std::unique_ptr<detail::ConnectionImpl> pimpl_;
};
}  // namespace cql
