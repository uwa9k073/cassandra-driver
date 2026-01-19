#pragma once

#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/crypto/openssl.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/tracing/scope_time.hpp>

namespace cassandra::protocol {
class Connection {
 public:
  Connection(userver::engine::TaskProcessor& tp,
             userver::concurrent::BackgroundTaskStorageCore& bts,
             userver::engine::SemaphoreLock&& pool_size_lock);

  void AsyncConnect(userver::engine::Deadline deadline,
                    userver::tracing::ScopeTime& scope);

 private:
  userver::engine::io::Socket _socket;
  userver::engine::TaskProcessor& bg_task_processor_;
  userver::concurrent::BackgroundTaskStorageCore& bg_task_storage_;

  void StartAsyncConnect();
  void WaitAsyncConnect(userver::engine::Deadline deadline);
};
}  // namespace cassandra::protocol
