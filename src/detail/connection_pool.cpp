#include "connection_pool.hpp"
#include <cstddef>
#include <memory>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <vector>
#include "cassandra/exception.hpp"
#include "cassandra/options.hpp"
#include "connection.hpp"

namespace cassandra::detail {

constexpr auto kUnlimitedConnecting = std::numeric_limits<std::size_t>::max();

constexpr std::chrono::seconds kConnectingTimeout{2};

ConnectionPool::ConnectionPool(
    NodeDescription description,
    userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& bg_task_processor,
    const std::string& keyspace,
    PoolSettings settings,
    ConnectionSettings connection_settings,
    userver::utils::statistics::MetricsStoragePtr metrics
)
    : _description(std::move(description)),
      _resolver(resolver),
      _keyspace(keyspace),
      _settings(settings),
      _connection_settings(connection_settings),
      _bg_task_processor(bg_task_processor),
      _queue(ConnectionQueue::Create()),
      _conn_consumer(_queue->GetMultiConsumer()),
      _conn_producer(_queue->GetMultiProducer()),
      size_semaphore_(settings.connecting_limit ? settings.connecting_limit : kUnlimitedConnecting),
      connecting_semaphore_(kUnlimitedConnecting),
      _metrics(std::move(metrics)) {}

std::shared_ptr<ConnectionPool> ConnectionPool::Create(
    NodeDescription description,
    userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& bg_task_processor,
    const std::string& keyspace,
    InitMode init_mode,
    PoolSettings settings,
    ConnectionSettings connection_settings,
    userver::utils::statistics::MetricsStoragePtr metrics
) {
    auto impl = std::make_shared<ConnectionPool>(
        description, resolver, bg_task_processor, keyspace, settings, connection_settings, metrics
    );

    impl->Init(init_mode);
    return impl;
}

void ConnectionPool::Init(InitMode init_mode) {
    auto settings = _settings.Read();

    if (settings->min_size > settings->max_size) {
        throw exceptions::InvalidConfig("Cassandra pool max size is less than requested initial size");
    }

    LOG_INFO(
        "{} initializing Cassandra connection pool, creating up to {} connections to {}:{}",
        init_mode == InitMode::kAsync ? "Asynchronously" : "Synchronously",
        settings->min_size,
        _description.contact_point.GetUnderlying(),
        _description.port.GetUnderlying()
    );

    std::vector<userver::engine::TaskWithResult<bool>> tasks;

    tasks.reserve(settings->min_size);

    const auto connection_settings = _connection_settings.ReadCopy();

    for (std::size_t i = 0; i < tasks.capacity(); ++i) {
        // Push connect task
        tasks.push_back(Connect(
            userver::engine::SemaphoreLock{size_semaphore_, std::try_to_lock}, ConnectionSettings{connection_settings}
        ));
    }

    if (init_mode == InitMode::kAsync) {
        for (auto& task : tasks) {
            _connect_task_storage.Detach(std::move(task));
        }
        LOG_INFO() << "Pool initialization is ongoing";
        return;
    }

    for (auto& t : tasks) {
        try {
            const auto success = t.Get();
            if (!success) {
                LOG_ERROR() << "Failed to establish connection to Cassandra server";
            }
        } catch (const std::exception& e) {
            LOG_ERROR(
                "Failed to establish connection with Cassandra server {}:{}: ",
                _description.contact_point.GetUnderlying(),
                _description.port.GetUnderlying()
            ) << e;
        }
    }
}

userver::engine::TaskWithResult<bool>
ConnectionPool::Connect(userver::engine::SemaphoreLock lock, ConnectionSettings&& conn_settings) {
    return userver::engine::AsyncNoSpan([this, size_lock = std::move(lock), conn_settings = std::move(conn_settings)](
                                        ) mutable {
        if (!size_lock) {
            size_lock = userver::engine::SemaphoreLock{size_semaphore_, kConnectingTimeout};
        }
        return DoConnect(std::move(size_lock), std::move(conn_settings));
    });
}
bool ConnectionPool::DoConnect(userver::engine::SemaphoreLock size_lock, ConnectionSettings&& conn_settings) {
    if (!size_lock) return false;
    LOG_TRACE() << "Creating Cassandra connection, current pool size: " << size_semaphore_.UsedApprox();

    const userver::engine::SemaphoreLock connecting_lock{connecting_semaphore_, kConnectingTimeout};
    if (!connecting_lock) {
        LOG_WARNING() << "Pool has too many establishing connections";
        return false;
    }

    std::unique_ptr<Connection> connection;

    try {
        connection = Connection::Connect(
            _description,
            _resolver,
            _bg_task_processor,
            _close_task_storage,
            std::move(conn_settings),
            std::move(size_lock),
            _metrics
        );
    } catch (...) {
        return false;
    }
    LOG_TRACE() << "Cassandra connection created";

    Push(connection.release());

    return true;
}

void ConnectionPool::Push(Connection* conn) {
    // Some cheks for validate connecetion
    //
    if (!_conn_producer.PushNoblock(std::move(conn))) {
        delete conn;
    }
}

Connection* ConnectionPool::Pop(userver::engine::Deadline deadline) {
    if (userver::engine::current_task::ShouldCancel()) {
        throw exceptions::PoolError("Task was cancelled before trying to get a connection");
    }

    if (deadline.IsReached()) {
        // ++stats_.connection.error_timeout;
        throw exceptions::PoolError("Deadline reached before trying to get a connection");
    }
    Connection* connection = nullptr;
    auto conn_settings = _connection_settings.Read();

    return connection;
}
}  // namespace cassandra::detail
