#pragma once

#include <cassandra/batch_query.hpp>
#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <memory>
#include <userver/cache/lru_map.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>

namespace cassandra::detail {
class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> {
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

    [[nodiscard]] ConnectionPtr Acquire(userver::engine::Deadline);
    void Release(Connection* connection);

    ResultSet Execute(
        Consistency level,
        const Query& query,
        const QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );

    ResultSet BatchExecute(
        const BatchQueryStore& store, OptionalCommandControl statement_cmd_ctl
    );

private:
    using RecentCounter = userver::utils::statistics::
        RecentPeriod<userver::utils::statistics::RelaxedCounter<size_t>, size_t>;

    void Init(InitMode mode);
    void Clear();

    void Push(Connection* connection);
    Connection* Pop(userver::engine::Deadline);

    void DeleteConnection(Connection* connection);
    void DropBrokenConnection(Connection* connection);
    void DropExpiredConnection(Connection* connection);
    void DropOutdatedConnection(Connection* connection);
    
    
    Connection* AcquireImmediate();

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

    // prepared statements cache
    // cassandra prepare statements per node and use ShortBytes as ID
    userver::cache::LruMap<std::string, io::ShortBytes> _prepared_statements_map;
    std::atomic<size_t> wait_count_;
    RecentCounter recent_conn_errors_;

    void TryCreateConnectionAsync();
    using ConnectionQueue = userver::concurrent::NonFifoMpmcQueue<Connection*>;
    using Consumer = ConnectionQueue::MultiConsumer;
    using Producer = ConnectionQueue::MultiProducer;

    std::shared_ptr<ConnectionQueue> _queue;
    Consumer _conn_consumer;
    Producer _conn_producer;
    userver::engine::Semaphore size_semaphore_;
    userver::engine::Semaphore connecting_semaphore_;

    userver::utils::statistics::MetricsStoragePtr _metrics;

    userver::utils::PeriodicTask _maintain_task;
    void Maintain();
    void StartMaintainTask();
};
}  // namespace cassandra::detail
