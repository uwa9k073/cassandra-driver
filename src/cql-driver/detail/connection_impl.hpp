#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>

#include <userver/engine/mutex.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/uuid4.hpp>

#include "../connection.hpp"
#include "../connection_state.hpp"
#include "../dsn.hpp"
#include "../prepared_statement.hpp"
#include "../prepared_statement_info.hpp"
#include "../settings.hpp"

namespace cql {
namespace protocol {
class Message;
class ResponseMessage;
class ErrorMessage;
class ResultMessage;
}  // namespace protocol

namespace detail {

/// @brief Implementation of CQL connection handling and protocol
class ConnectionImpl {
 public:
  ConnectionImpl(
      ::userver::engine::TaskProcessor& bg_task_processor,
      ::userver::concurrent::BackgroundTaskStorageCore& bg_task_storage,
      uint32_t id, const ConnectionSettings& settings,
      ::userver::utils::statistics::MetricsStoragePtr metrics);

  ~ConnectionImpl();

  void AsyncConnect(const Dsn& dsn, ::userver::engine::Deadline deadline);
  void Close();

  bool IsConnected() const;
  bool IsHealthy() const;
  ConnectionState GetState() const;
  int GetServerVersion() const;

  ResultSet Execute(const Query& query, const QueryParameters& params,
                    ::userver::engine::Deadline deadline);

  PreparedStatement Prepare(const Query& query,
                            ::userver::engine::Deadline deadline);

  ResultSet ExecutePrepared(const PreparedStatement& stmt,
                            const QueryParameters& params,
                            ::userver::engine::Deadline deadline);

  // const Statistics& GetStatistics() const;
  const ConnectionSettings& GetSettings() const;

 private:
  void SendMessage(const protocol::Message& msg,
                   ::userver::engine::Deadline deadline);
  protocol::ResponseMessage ReceiveMessage(
      ::userver::engine::Deadline deadline);

  void HandleStartup(::userver::engine::Deadline deadline);
  void HandleAuthentication(::userver::engine::Deadline deadline);
  void Handshake(::userver::engine::Deadline deadline);

  void UpdateSocketState();
  void CheckConnection() const;
  void MarkAsBroken();

  bool WaitSocketReadable(::userver::engine::Deadline deadline);
  bool WaitSocketWriteable(::userver::engine::Deadline deadline);

  void ProcessError(const protocol::ErrorMessage& error);
  ResultSet ProcessResult(protocol::ResultMessage&& result);

  const PreparedStatementInfo& GetPreparedInfo(
      const Query& query, ::userver::engine::Deadline deadline);
  void CleanupPreparedStatements(::userver::engine::Deadline deadline);

 private:
  const std::string uuid_;
  userver::engine::TaskProcessor& bg_task_processor_;
  userver::concurrent::BackgroundTaskStorageCore& bg_task_storage_;

  ::userver::engine::Mutex mutex_;
  ::userver::engine::io::Socket socket_;
  ConnectionSettings settings_;
  ConnectionState state_{ConnectionState::Status::NOT_CONNECTED};

  std::unordered_map<std::string, PreparedStatementInfo> prepared_statements_;
  int server_version_{0};
  bool compression_enabled_{false};

  // Statistics stats_;
  ::userver::utils::statistics::MetricsStoragePtr metrics_;

  std::vector<std::uint8_t> read_buffer_;
  std::vector<std::uint8_t> write_buffer_;

  static constexpr std::size_t kInitialBufferSize = 8192;
  static constexpr std::size_t kMaxMessageSize = 256 * 1024 * 1024;  // 256MB
  static constexpr auto kMinConnectTimeout = std::chrono::milliseconds(500);
};  // class ConnectionImpl

}  // namespace detail
}  // namespace cql
