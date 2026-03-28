#pragma once

#include <cassandra/node_description.hpp>
#include <cassandra/result_set.hpp>
#include <detail/connection.hpp>

#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <detail/stream_pool.hpp>
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
namespace cassandra::detail {
class ConnectionImpl {
    using RecvMessageQueue = userver::concurrent::SpscQueue<
        std::shared_ptr<io::protocol::ResponseMessage>>;

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

    bool IsBroken() const { return _broken.load(std::memory_order_relaxed); }

    ResultSet Execute(
        Consistency level,
        const Query& query,
        const QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );
    ResultSet ExecutePrepared(
        Consistency level,
        const io::ShortBytes& statement_id,
        const QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );

    io::ShortBytes Prepare(const io::LongString& query_name);

    ~ConnectionImpl();

private:
    userver::engine::io::Socket _socket;
    userver::engine::TaskProcessor& bg_task_processor_;
    userver::concurrent::BackgroundTaskStorageCore& bg_task_storage_;
    ConnectionSettings settings_;
    std::optional<std::chrono::steady_clock::time_point> expires_at_;
    userver::engine::SemaphoreLock pool_size_lock_;
    userver::utils::statistics::MetricsStoragePtr _metrics;
    StreamPool _stream_pool;
    userver::engine::Mutex _send_mutex;
    io::protocol::CompressorPtr _compressor_ptr;

    void TcpConnect(
        const userver::clients::dns::AddrVector& addresses,
        userver::engine::Deadline deadline
    );
    void CqlHandshake(bool use_compression);
    void StartReceiverLoop();

    void SendMessage(
        io::protocol::RequestMessage&& message, userver::engine::Deadline deadline
    );

    void SendMessage(
        std::unique_ptr<io::protocol::RequestMessage> message,
        userver::engine::Deadline deadline
    );
    std::shared_ptr<io::protocol::ResponseMessage> ReadFrame(
        userver::engine::Deadline deadline
    );

    std::shared_ptr<io::protocol::ResponseMessage> ExecuteMessage(
        io::protocol::RequestMessage&& message, userver::engine::Deadline deadline
    );

    std::shared_ptr<io::protocol::ResponseMessage> ExecuteMessageAsync(
        std::unique_ptr<io::protocol::RequestMessage> message,
        userver::engine::Deadline deadline
    );

    void ReceiverLoop();

    void MarkBroken();

    userver::engine::Task Close();

    std::vector<std::shared_ptr<RecvMessageQueue>> _received_message_queue_map;
    std::vector<RecvMessageQueue::Producer> _received_message_producer_map;
    std::vector<RecvMessageQueue::Consumer> _received_message_consumer_map;

    std::atomic<bool> _broken;
    userver::engine::Task _receiver_task;
};
}  // namespace cassandra::detail
