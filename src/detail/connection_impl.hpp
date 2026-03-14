#pragma once

#include <cassandra/node_description.hpp>
#include <cassandra/result_set.hpp>
#include <detail/connection.hpp>

#include <cassandra/io/protocol/lz4_utils.hpp>
#include <io/protocol/request_message.hpp>
#include <io/protocol/response_message.hpp>
#include <userver/clients/dns/common.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/statistics/fwd.hpp>

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

    void AsyncConnect(
        userver::clients::dns::AddrVector addresses,
        bool use_compression,
        userver::engine::Deadline deadline
    );

    bool IsExpired() const {
        return expires_at_.has_value() &&
               userver::utils::datetime::SteadyNow() > expires_at_;
    }

    ResultSet Execute(
        Consistency level,
        const Query& query,
        const QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );

    ~ConnectionImpl();

private:
    userver::engine::io::Socket _socket;
    userver::engine::TaskProcessor& bg_task_processor_;
    userver::concurrent::BackgroundTaskStorageCore& bg_task_storage_;
    ConnectionSettings settings_;
    std::optional<std::chrono::steady_clock::time_point> expires_at_;
    userver::engine::SemaphoreLock pool_size_lock_;
    userver::utils::statistics::MetricsStoragePtr _metrics;

    void StartAsyncConnect(ContactPoint contact_point, Port port);
    void WaitAsyncConnect(
        userver::engine::Deadline deadline, ContactPoint contact_point, Port port
    );

    void SendMessage(io::protocol::RequestMessage&& message);
    std::shared_ptr<io::protocol::ResponseMessage> WaitForResult();

    userver::engine::Task Close();

    io::protocol::CompressorPtr _compressor_ptr;
};
}  // namespace cassandra::detail
