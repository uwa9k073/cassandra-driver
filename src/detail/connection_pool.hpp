#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <memory>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

namespace cassandra::detail {
class ConnectionPool {
public:
    ConnectionPool(
        NodeDescription description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& bg_task_processor,
        const std::string& keyspace,
        PoolSettings settings,
        ConnectionSettings connection_settings,
        userver::utils::statistics::MetricsStoragePtr metrics
    );

    static std::shared_ptr<ConnectionPool> Create(
        NodeDescription description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& bg_task_processor,
        const std::string& keyspace,
        InitMode init_mode,
        PoolSettings settings,
        ConnectionSettings connection_settings,
        userver::utils::statistics::MetricsStoragePtr metrics
    );

    ~ConnectionPool();

    std::shared_ptr<StreamPool> GetStreamPool();

private:
    using RecentCounter = userver::utils::statistics::
        RecentPeriod<userver::utils::statistics::RelaxedCounter<size_t>, size_t>;

    void Init(InitMode mode);
    void Clear();

    void Push(Connection* connection);
    Connection* Pop(userver::engine::Deadline);

    void DeleteConnection(Connection* connection);
    void DeleteBrokenConnection(Connection* connection);
    void DropExpiredConnection(Connection* connection);
    void DropOutdatedConnection(Connection* connection);

    [[nodiscard]] userver::engine::TaskWithResult<bool> Connect(
        userver::engine::SemaphoreLock lock, ConnectionSettings&& conn_settings
    );
    bool DoConnect(userver::engine::SemaphoreLock, ConnectionSettings&&);

    NodeDescription _description;
    userver::clients::dns::Resolver* _resolver;

    std::string _keyspace;
    userver::rcu::Variable<PoolSettings> _settings;
    userver::rcu::Variable<ConnectionSettings> _connection_settings;
    userver::engine::TaskProcessor& _bg_task_processor;
    userver::concurrent::BackgroundTaskStorageCore _connect_task_storage;
    userver::concurrent::BackgroundTaskStorageCore _close_task_storage;

    std::atomic<size_t> wait_count_;
    RecentCounter recent_conn_errors_;

    void TryCreateConnectionAsync();
    using ConnectionQueue = userver::concurrent::NonFifoMpmcQueue<Connection*>;
    using Consumer = ConnectionQueue::MultiConsumer;
    using Producer = ConnectionQueue::MultiProducer;

    std::shared_ptr<StreamPool> _stream_pool_ptr;
    std::shared_ptr<ConnectionQueue> _queue;
    Consumer _conn_consumer;
    Producer _conn_producer;
    userver::engine::Semaphore size_semaphore_;
    userver::engine::Semaphore connecting_semaphore_;

    userver::utils::statistics::MetricsStoragePtr _metrics;
};
}  // namespace cassandra::detail
