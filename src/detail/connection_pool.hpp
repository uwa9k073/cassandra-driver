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

private:
    void Init(InitMode mode);
    void Push(Connection* connection);
    Connection* Pop(userver::engine::Deadline);

    [[nodiscard]] userver::engine::TaskWithResult<bool>
    Connect(userver::engine::SemaphoreLock lock, ConnectionSettings&& conn_settings);
    bool DoConnect(userver::engine::SemaphoreLock, ConnectionSettings&&);

    NodeDescription _description;
    userver::clients::dns::Resolver* _resolver;

    std::string _keyspace;
    userver::rcu::Variable<PoolSettings> _settings;
    userver::rcu::Variable<ConnectionSettings> _connection_settings;
    userver::engine::TaskProcessor& _bg_task_processor;
    userver::concurrent::BackgroundTaskStorageCore _connect_task_storage;
    userver::concurrent::BackgroundTaskStorageCore _close_task_storage;

    using ConnectionQueue = userver::concurrent::NonFifoMpmcQueue<Connection*>;
    using Consumer = ConnectionQueue::MultiConsumer;
    using Producer = ConnectionQueue::MultiProducer;
    std::shared_ptr<ConnectionQueue> _queue;
    Consumer _conn_consumer;
    Producer _conn_producer;
    userver::engine::Semaphore size_semaphore_;
    userver::engine::Semaphore connecting_semaphore_;

    userver::utils::statistics::MetricsStoragePtr _metrics;
};
}  // namespace cassandra::detail
