#include <algorithm>
#include <cassandra/batch_query.hpp>
#include <cassandra/detail/connection_ptr.hpp>
#include <cassandra/exception.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <cstddef>
#include <detail/connection.hpp>
#include <detail/connection_pool.hpp>
#include <detail/stream_pool.hpp>
#include <iterator>
#include <memory>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <vector>

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
      _prepared_statements_map(100),
      _queue(ConnectionQueue::Create()),
      _conn_consumer(_queue->GetMultiConsumer()),
      _conn_producer(_queue->GetMultiProducer()),
      size_semaphore_(
          settings.connecting_limit ? settings.connecting_limit
                                    : kUnlimitedConnecting
      ),
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
        description,
        resolver,
        bg_task_processor,
        keyspace,
        settings,
        connection_settings,
        metrics
    );

    impl->Init(init_mode);
    return impl;
}

void ConnectionPool::Init(InitMode init_mode) {
    auto settings = _settings.Read();

    if (settings->min_size > settings->max_size) {
        throw exceptions::InvalidConfig(
            "Cassandra pool max size is less than requested initial "
            "size"
        );
    }

    LOG_INFO(
        "{} initializing Cassandra connection pool, creating up to "
        "{} "
        "connections to {}:{}",
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
            userver::engine::SemaphoreLock{size_semaphore_, std::try_to_lock},
            ConnectionSettings{connection_settings}
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
                LOG_ERROR() << "Failed to establish connection to "
                               "Cassandra server";
            }
        } catch (const std::exception& e) {
            LOG_ERROR(
                "Failed to establish connection with Cassandra "
                "server {}:{}: ",
                _description.contact_point.GetUnderlying(),
                _description.port.GetUnderlying()
            ) << e;
        }
    }
}

userver::engine::TaskWithResult<bool> ConnectionPool::Connect(
    userver::engine::SemaphoreLock lock, ConnectionSettings&& conn_settings
) {
    return userver::engine::AsyncNoSpan([this,
                                         size_lock = std::move(lock),
                                         conn_settings =
                                             std::move(conn_settings)]() mutable {
        if (!size_lock) {
            size_lock =
                userver::engine::SemaphoreLock{size_semaphore_, kConnectingTimeout};
        }
        return DoConnect(std::move(size_lock), std::move(conn_settings));
    });
}
bool ConnectionPool::DoConnect(
    userver::engine::SemaphoreLock size_lock, ConnectionSettings&& conn_settings
) {
    if (!size_lock) return false;
    LOG_TRACE() << "Creating Cassandra connection, current pool size: "
                << size_semaphore_.UsedApprox();

    const userver::engine::SemaphoreLock connecting_lock{
        connecting_semaphore_, kConnectingTimeout
    };
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

    auto conn_settings = _connection_settings.Read();
    if (conn->IsExpired()) {
        DropExpiredConnection(conn);
        return;
    }

    LOG_DEBUG("PUSH TO QUEUE");

    if (!_conn_producer.PushNoblock(std::move(conn))) {
        LOG_WARNING("Couldn't push connection back to the pool. Deleting...");
        DeleteConnection(conn);
    }

    LOG_DEBUG("SUCCESS PUSH TO QUEUE");
}

constexpr std::chrono::seconds kRecentErrorPeriod{15};
constexpr auto kPendingConnectsMax{1};
void ConnectionPool::TryCreateConnectionAsync() {
    auto conn_settings = _connection_settings.ReadCopy();
    // Checking errors is more expensive than incrementing an atomic,
    // so we check it only if we can start a new connection.
    if (recent_conn_errors_.GetStatsForPeriod(kRecentErrorPeriod, true) <
        conn_settings.recent_errors_threshold) {
        userver::engine::SemaphoreLock size_lock{size_semaphore_, std::try_to_lock};
        if (size_lock ||
            _connect_task_storage.ActiveTasksApprox() <= kPendingConnectsMax) {
            _connect_task_storage.Detach(
                Connect(std::move(size_lock), std::move(conn_settings))
            );
        }
    } else {
        LOG_DEBUG() << "Too many connection errors in recent period";
    }
}

Connection* ConnectionPool::Pop(userver::engine::Deadline deadline) {
    if (userver::engine::current_task::ShouldCancel()) {
        throw exceptions::PoolError(
            "Task was cancelled before trying to get a connection"
        );
    }

    if (deadline.IsReached()) {
        // ++stats_.connection.error_timeout;
        throw exceptions::PoolError(
            "Deadline reached before trying to get a connection"
        );
    }
    Connection* connection = nullptr;
    auto conn_settings = _connection_settings.Read();

    while (_conn_consumer.PopNoblock(connection)) {
        if (connection->IsExpired()) {
            DropExpiredConnection(connection);
            continue;
        } else if (connection->IsBroken()) {
            DeleteBrokenConnection(connection);
            continue;
        }
        return connection;
    }

    TryCreateConnectionAsync();
    if (_conn_consumer.Pop(connection, deadline)) {
        return connection;
    }
    if (userver::engine::current_task::ShouldCancel()) {
        throw exceptions::PoolError("Task was cancelled while waiting for connection"
        );
    }

    throw exceptions::PoolError(
        fmt::format(
            "No available connections found. Connecting: {}. Max "
            "concurrent "
            "connecting: {}. Active: {}. Max active {}",
            connecting_semaphore_.UsedApprox(),
            connecting_semaphore_.GetCapacity(),
            size_semaphore_.UsedApprox(),
            size_semaphore_.GetCapacity()
        ),
        _keyspace
    );
}

ConnectionPool::~ConnectionPool() { Clear(); }

void ConnectionPool::Clear() {
    Connection* connection = nullptr;
    while (_conn_consumer.PopNoblock(connection)) {
        delete connection;
    }
    _close_task_storage.CancelAndWait();
}

void ConnectionPool::DeleteConnection(Connection* connection) {
    // stats incrementing
    delete connection;
}

void ConnectionPool::DeleteBrokenConnection(Connection* connection) {
    LOG_WARNING("Released connection in closed state. Deleting...");
    DeleteConnection(connection);
}

void ConnectionPool::DropExpiredConnection(Connection* connection) {
    LOG_INFO("Dropping expired connection");
    DeleteConnection(connection);
}

void ConnectionPool::DropOutdatedConnection(Connection* connection) {
    LOG_INFO("Dropping outdated connection");
    DeleteConnection(connection);
}

[[nodiscard]] ConnectionPtr ConnectionPool::Acquire(
    userver::engine::Deadline deadline
) {
    auto shared_this = shared_from_this();

    // auto config = GetConfigSource().GetSnapshot();
    // CheckDeadlineIsExpired(config);
    ConnectionPtr connection{Pop(deadline), std::move(shared_this)};
    // ++stats_.connection.used;
    // CheckDeadlineIsExpired(config);

    // connection->UpdateDefaultCommandControl();
    return connection;
}

void ConnectionPool::Release(Connection* connection) {
    // UASSERT(connection);
    // using DecGuard =
    // storages::postgres::SizeGuard<USERVER_NAMESPACE::utils::statistics::RelaxedCounter<uint32_t>>;
    // DecGuard dg{stats_.connection.used, DecGuard::DontIncrement{}};

    // std::optional<Connection::Statistics> connection_stats{};
    // Grab stats only if connection is not in transaction
    // if (!connection->IsInTransaction()) {
    //     connection_stats.emplace(connection->GetStatsAndReset());
    // }
    if (connection->IsExpired()) {
        DropExpiredConnection(connection);
    } else if (connection->IsBroken()) {
        DeleteBrokenConnection(connection);
    } else {
        LOG_DEBUG("PUSHING CONNECTION");
        Push(connection);
    }
    // else {
    //     // Connection cleanup is done asynchronously while returning control to
    //     // the user
    //     close_task_storage_.Detach(USERVER_NAMESPACE::utils::CriticalAsync(
    //         "clear_conn_after_cancel",
    //         [this, connection, dec_cnt = std::move(dg)] {
    //             LOG_LIMITED_WARNING() << "Released connection in busy state.
    //             Trying to clean up..."; TESTPOINT("pg_cleanup",
    //             formats::json::Value{}); CleanupConnection(connection);
    //         }
    //     ));
    // }

    // // We want to account the stats AFTER the connection is returned to the pool,
    // // because the procedure is somewhat heavy and there's no point to prevent the
    // // connection from being reused
    // if (connection_stats.has_value()) {
    //     AccountConnectionStats(std::move(*connection_stats));
    // }
}

ResultSet ConnectionPool::Execute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    auto conn = Acquire(userver::engine::Deadline{});

    if (!statement_cmd_ctl.has_value() ||
        statement_cmd_ctl->prepared_statements_enabled ==
            CommandControl::PreparedStatementsOptionOverride::kNoOverride) {
        LOG_DEBUG("TRYING TO GET PREPARED");
        auto prepared_id_ptr =
            _prepared_statements_map.Get(query.GetStatement().GetUnderlying());
        if (prepared_id_ptr) {
            LOG_DEBUG("FOUND PREPARED");

            return conn->ExecutePrepared(
                level, *prepared_id_ptr, params, statement_cmd_ctl
            );
        } else {
            LOG_DEBUG("PREPARE");
            auto prepared_id = conn->Prepare(query.GetStatement());
            _prepared_statements_map.Put(
                query.GetStatement().GetUnderlying(), prepared_id
            );

            return conn->ExecutePrepared(
                level, prepared_id, params, statement_cmd_ctl
            );
        }
    }
    return conn->Execute(level, query, params, statement_cmd_ctl);
}

ResultSet ConnectionPool::BatchExecute(
    const BatchQueryStore& store, OptionalCommandControl statement_cmd_ctl
) {
    auto conn = Acquire(userver::engine::Deadline{});
    auto queries_view = store.Queries();
    std::vector<BatchStatement> batch_statements;
    batch_statements.reserve(queries_view.size());

    if (!statement_cmd_ctl.has_value() ||
        statement_cmd_ctl->prepared_statements_enabled ==
            CommandControl::PreparedStatementsOptionOverride::kNoOverride) {
        std::ranges::transform(
            queries_view,
            std::back_inserter(batch_statements),
            [this, &conn](const BatchQuery& el) {
                auto statement = el.GetQuery().GetStatement().GetUnderlying();
                auto prepared_id_ptr = this->_prepared_statements_map.Get(statement);
                if (prepared_id_ptr) {
                    return BatchStatement{*prepared_id_ptr, el.GetParams()};
                } else {
                    auto prepared_id = conn->Prepare(el.GetQuery().GetStatement());
                    this->_prepared_statements_map.Put(statement, prepared_id);
                    return BatchStatement{prepared_id, el.GetParams()};
                }
            }
        );
    } else {
        std::ranges::transform(
            queries_view,
            std::back_inserter(batch_statements),
            [](const BatchQuery& el) {
                return BatchStatement{el.GetQuery().GetStatement(), el.GetParams()};
            }
        );
    }
    return conn->BatchExecute(
        store.ConsistencyLevel(), batch_statements, statement_cmd_ctl
    );
}

}  // namespace cassandra::detail
