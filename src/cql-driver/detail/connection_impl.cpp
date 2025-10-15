#include "connection_impl.hpp"

#include <userver/engine/io/socket.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <chrono>
#include <cql-driver/protocol/messages.hpp>

namespace cql::detail {

namespace {

constexpr auto kConnectTimeout = std::chrono::milliseconds(1000);
constexpr auto kSocketTimeout = std::chrono::milliseconds(200);

}  // namespace

ConnectionImpl::ConnectionImpl(
    userver::engine::TaskProcessor& bg_task_processor,
    userver::concurrent::BackgroundTaskStorageCore& bg_task_storage,
    uint32_t id, const ConnectionSettings& settings,
    USERVER_NAMESPACE::utils::statistics::MetricsStoragePtr metrics)
    : uuid_(USERVER_NAMESPACE::utils::generators::GenerateUuid()),
      bg_task_processor_(bg_task_processor),
      bg_task_storage_(bg_task_storage),
      settings_(settings),
      metrics_(std::move(metrics)),
      read_buffer_(kInitialBufferSize),
      write_buffer_(kInitialBufferSize) {
  LOG_DEBUG() << "Creating CQL connection " << id;
}

ConnectionImpl::~ConnectionImpl() { Close(); }

void ConnectionImpl::AsyncConnect(const Dsn& dsn,
                                  userver::engine::Deadline deadline) {
  auto timeout =
      std::min(deadline.TimeLeft(),
               std::max(kMinConnectTimeout, settings_.connect_timeout));

  LOG_INFO() << "Connecting to " << dsn.GetHost() << ":" << dsn.GetPort();

  try {
    socket_.Connect(dsn.GetHost(), dsn.GetPort(), timeout);
    state_ = ConnectionState::Status::CONNECTING;

    Handshake(deadline);
    state_ = ConnectionState::Status::CONNECTED;

    LOG_INFO() << "Connected successfully to " << dsn.GetHost() << ":"
               << dsn.GetPort();
  } catch (const std::exception& e) {
    state_ = ConnectionState::Status::ERROR;
    LOG_ERROR() << "Connection failed: " << e.what();
    throw;
  }
}

void ConnectionImpl::Close() {
  if (IsConnected()) {
    LOG_INFO() << "Closing connection " << uuid_;
    socket_.Close();
    state_ = ConnectionState::Status::NOT_CONNECTED;
  }
}

void ConnectionImpl::Handshake(userver::engine::Deadline deadline) {
  // Send STARTUP message
  protocol::StartupMessage startup;
  SendMessage(startup, deadline);

  // Wait for response
  auto response = ReceiveMessage(deadline);

  switch (response.GetType()) {
    case protocol::MessageType::kReady:
      // Connection ready
      break;

    case protocol::MessageType::kAuthenticate:
      HandleAuthentication(deadline);
      break;

    case protocol::MessageType::kError:
      ProcessError(protocol::ErrorMessage(std::move(response)));
      break;

    default:
      throw ConnectionError(
          "Unexpected response type during handshake: " +
          std::to_string(static_cast<int>(response.GetType())));
  }
}

void ConnectionImpl::HandleAuthentication(engine::Deadline deadline) {
  // Implement authentication logic here based on settings_.auth_type
  // For now just throw not implemented
  throw NotImplementedError("Authentication not implemented yet");
}

void ConnectionImpl::SendMessage(const protocol::Message& msg,
                                 engine::Deadline deadline) {
  write_buffer_.clear();
  msg.Serialize(write_buffer_);

  std::size_t bytes_sent = 0;
  while (bytes_sent < write_buffer_.size()) {
    if (!WaitSocketWriteable(deadline)) {
      throw ConnectionTimeoutError("Write operation timed out");
    }

    const auto chunk =
        socket_.WriteAll(write_buffer_.data() + bytes_sent,
                         write_buffer_.size() - bytes_sent, deadline);

    bytes_sent += chunk;
  }
}

protocol::ResponseMessage ConnectionImpl::ReceiveMessage(
    userver::engine::Deadline deadline) {
  // First read the header (9 bytes)
  static constexpr std::size_t kHeaderSize = 9;
  read_buffer_.resize(kHeaderSize);

  std::size_t bytes_read = 0;
  while (bytes_read < kHeaderSize) {
    if (!WaitSocketReadable(deadline)) {
      throw ConnectionTimeoutError("Read operation timed out");
    }

    const auto chunk = socket_.ReadAll(read_buffer_.data() + bytes_read,
                                       kHeaderSize - bytes_read, deadline);

    if (chunk == 0) {
      throw ConnectionError("Connection closed by peer");
    }

    bytes_read += chunk;
  }

  // Parse header
  protocol::MessageHeader header;
  header.Deserialize(read_buffer_.data());

  // Read body
  const auto body_size = header.GetBodyLength();
  if (body_size > kMaxMessageSize) {
    throw ProtocolError("Message too large: " + std::to_string(body_size) +
                        " bytes (max: " + std::to_string(kMaxMessageSize) +
                        ")");
  }

  read_buffer_.resize(kHeaderSize + body_size);
  bytes_read = kHeaderSize;

  while (bytes_read < read_buffer_.size()) {
    if (!WaitSocketReadable(deadline)) {
      throw ConnectionTimeoutError("Read operation timed out");
    }

    const auto chunk =
        socket_.ReadAll(read_buffer_.data() + bytes_read,
                        read_buffer_.size() - bytes_read, deadline);

    if (chunk == 0) {
      throw ConnectionError("Connection closed by peer");
    }

    bytes_read += chunk;
  }

  return protocol::ResponseMessage(std::move(header), read_buffer_);
}

bool ConnectionImpl::WaitSocketReadable(userver::engine::Deadline deadline) {
  return socket_.WaitReadable(deadline);
}

bool ConnectionImpl::WaitSocketWriteable(userver::engine::Deadline deadline) {
  return socket_.WaitWriteable(deadline);
}

ResultSet ConnectionImpl::Execute(const Query& query,
                                  const QueryParameters& params,
                                  userver::engine::Deadline deadline) {
  CheckConnection();

  if (settings_.use_prepared_statements) {
    const auto& prepared = GetPreparedInfo(query, deadline);
    return ExecutePrepared(prepared, params, deadline);
  }

  // Send query
  protocol::QueryMessage msg(query.GetText(), params);
  SendMessage(msg, deadline);

  // Get response
  auto response = ReceiveMessage(deadline);

  if (response.GetType() == protocol::MessageType::kError) {
    ProcessError(protocol::ErrorMessage(std::move(response)));
  }

  UASSERT(response.GetType() == protocol::MessageType::kResult);
  return ProcessResult(protocol::ResultMessage(std::move(response)));
}

void ConnectionImpl::ProcessError(const protocol::ErrorMessage& error) {
  // Log and convert CQL error into appropriate exception
  LOG_ERROR() << "CQL error: [" << error.GetCode() << "] "
              << error.GetMessage();

  switch (error.GetCode()) {
    case 0x1000:  // Server error
      throw ServerError(error.GetMessage());
    case 0x2000:  // Protocol error
      throw ProtocolError(error.GetMessage());
    // Add other error codes
    default:
      throw QueryError(error.GetMessage());
  }
}

ResultSet ConnectionImpl::ProcessResult(protocol::ResultMessage&& result) {
  switch (result.GetKind()) {
    case protocol::ResultKind::kRows:
      return ResultSet(std::move(result));
    case protocol::ResultKind::kSetKeyspace:
      // Handle keyspace change
      return ResultSet();
    case protocol::ResultKind::kSchemaChange:
      // Handle schema change
      return ResultSet();
    default:
      return ResultSet();
  }
}

void ConnectionImpl::CheckConnection() const {
  if (!IsConnected()) {
    throw ConnectionError("Not connected");
  }
  if (!IsHealthy()) {
    throw ConnectionError("Connection is not healthy");
  }
}

bool ConnectionImpl::IsConnected() const {
  return state_ == ConnectionState::kConnected;
}

bool ConnectionImpl::IsHealthy() const {
  return IsConnected() && socket_.IsValid();
}

void ConnectionImpl::MarkAsBroken() {
  state_ = ConnectionState::kError;
  Close();
}

ConnectionState ConnectionImpl::GetState() const { return state_; }

const Statistics& ConnectionImpl::GetStatistics() const { return stats_; }

const ConnectionSettings& ConnectionImpl::GetSettings() const {
  return settings_;
}

}  // namespace cql::detail
