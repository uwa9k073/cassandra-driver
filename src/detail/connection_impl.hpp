#pragma once

#include <cassandra/node_description.hpp>
#include <cassandra/result_set.hpp>
#include <cstdint>
#include <detail/connection.hpp>

#include <cassandra/io/protocol/lz4_utils.hpp>
#include <io/protocol/request_message.hpp>
#include <io/protocol/response_message.hpp>
#include <memory>
#include <userver/clients/dns/common.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <vector>
#include "detail/stream_pool.hpp"

namespace cassandra::detail {
class ConnectionImpl {
    using MessageFuture =
        userver::engine::Future<std::shared_ptr<io::protocol::ResponseMessage>>;
    using MessagePromise =
        userver::engine::Promise<std::shared_ptr<io::protocol::ResponseMessage>>;

    using RecvMessageQueue = userver::concurrent::SpscQueue<
        std::shared_ptr<io::protocol::ResponseMessage>>;

public:
    ConnectionImpl(
        userver::engine::TaskProcessor& tp,
        userver::concurrent::BackgroundTaskStorageCore& bts,
        ConnectionSettings settings,
        userver::engine::SemaphoreLock&& pool_size_lock,
        userver::utils::statistics::MetricsStoragePtr metrics,
        std::shared_ptr<StreamPool> stream_pool_ptr
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
    void WaitForResult(MessagePromise&& promise, std::int16_t stream_id = 0);
    std::shared_ptr<io::protocol::ResponseMessage> ReadFrame();

    userver::engine::Task Close();

    std::shared_ptr<StreamPool> _stream_pool_ptr;

    io::protocol::CompressorPtr _compressor_ptr;
    userver::engine::Mutex _message_queue_mutex;
    userver::engine::ConditionVariable _stream_ready_cv;

    std::vector<std::pair<bool, MessagePromise>> _receiving_promise_map;

    std::vector<std::shared_ptr<RecvMessageQueue>> _received_message_queue_map;
    std::vector<RecvMessageQueue::Producer> _received_message_producer_map;
    std::vector<RecvMessageQueue::Consumer> _received_message_consumer_map;
    std::size_t _receiving_promise_count;
};
}  // namespace cassandra::detail
