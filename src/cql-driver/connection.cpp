#include "connection.hpp"
#include "detail/connection_impl.hpp"

namespace cql {

Connection::Connection(engine::TaskProcessor& bg_task_processor,
                       concurrent::BackgroundTaskStorageCore& bg_task_storage,
                       uint32_t id, const ConnectionSettings& settings)
    : pimpl_(std::make_unique<detail::ConnectionImpl>(
          bg_task_processor, bg_task_storage, id, settings,
          nullptr  // metrics will be set later if needed
          )) {}

Connection::~Connection() = default;

void Connection::Connect(const Dsn& dsn, userver::engine::Deadline deadline) {
  pimpl_->AsyncConnect(dsn, deadline);
}

void Connection::Close() { pimpl_->Close(); }

bool Connection::IsConnected() const { return pimpl_->IsConnected(); }

bool Connection::IsHealthy() const { return pimpl_->IsHealthy(); }

ResultSet Connection::Execute(const Query& query, const QueryParameters& params,
                              userver::engine::Deadline deadline) {
  return pimpl_->Execute(query, params, deadline);
}

PreparedStatement Connection::Prepare(const Query& query,
                                      userver::engine::Deadline deadline) {
  return pimpl_->Prepare(query, deadline);
}

ResultSet Connection::ExecutePrepared(const PreparedStatement& stmt,
                                      const QueryParameters& params,
                                      engine::Deadline deadline) {
  return pimpl_->ExecutePrepared(stmt, params, deadline);
}

const Statistics& Connection::GetStatistics() const {
  return pimpl_->GetStatistics();
}

const ConnectionSettings& Connection::GetSettings() const {
  return pimpl_->GetSettings();
}

int Connection::GetServerVersion() const { return pimpl_->GetServerVersion(); }

ConnectionState Connection::GetState() const { return pimpl_->GetState(); }
