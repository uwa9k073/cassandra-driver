#pragma once

#include <cassandra/node_description.hpp>
#include "connection.hpp"

#include <userver/clients/dns/common.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include "../io/protocol/request_message.hpp"

namespace cassandra::detail {
class ConnectionImpl {
public:
    ConnectionImpl(
        userver::engine::TaskProcessor& tp,
        userver::concurrent::BackgroundTaskStorageCore& bts,
        ConnectionSettings settings,
        userver::engine::SemaphoreLock&& pool_size_lock,
        userver::utils::statistics::MetricsStoragePtr metrics
    );

    void AsyncConnect(userver::clients::dns::AddrVector addresses, userver::engine::Deadline deadline);

private:
    userver::engine::io::Socket _socket;
    userver::engine::TaskProcessor& bg_task_processor_;
    userver::concurrent::BackgroundTaskStorageCore& bg_task_storage_;
    ConnectionSettings settings_;
    std::optional<std::chrono::steady_clock::time_point> expires_at_;
    userver::engine::SemaphoreLock pool_size_lock_;
    userver::utils::statistics::MetricsStoragePtr _metrics;

    void StartAsyncConnect(ContactPoint contact_point, Port port);
    void WaitAsyncConnect(userver::engine::Deadline deadline, ContactPoint contact_point, Port port);

    void SendMessage(io::protocol::RequestMessage&& message);
    void WaitForResult();
};
}  // namespace cassandra::detail
