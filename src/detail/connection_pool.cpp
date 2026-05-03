#include <algorithm>
#include <cassandra/batch_query.hpp>
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
#include <optional>
#include <ranges>
#include <span>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/periodic_task.hpp>
#include <vector>
#include "cassandra/detail/query_parameters.hpp"

namespace cassandra::detail {

namespace {
bool CanPrepareStatement(
    OptionalCommandControl command_control, bool prepared_statement_cache_enabled
) {
    if (command_control.has_value()) {
        if (auto mode = command_control.value().prepared_statements_enabled;
            mode != CommandControl::PreparedStatementsOptionOverride::kNoOverride)
            return mode ==
                   CommandControl::PreparedStatementsOptionOverride::kEnabled;
    }

    return prepared_statement_cache_enabled;
}
}  // namespace

constexpr std::chrono::seconds kMaintainInterval{30};
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
      _prepared_statements_map(
          settings.prepared_statement_cache_ways,
          settings.prepared_statement_cache_way_size
      ),
      _size_semaphore(
          settings.connecting_limit ? settings.connecting_limit
                                    : kUnlimitedConnecting
      ),
      _connecting_semaphore(kUnlimitedConnecting),
      _metrics(std::move(metrics)),
      _prepared_statements_cache_enabled(settings.prepared_statement_cache_enabled) {
}

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
            userver::engine::SemaphoreLock{_size_semaphore, std::try_to_lock},
            ConnectionSettings{connection_settings}
        ));
    }

    if (init_mode == InitMode::kAsync) {
        for (auto& task : tasks) {
            _connect_task_storage.Detach(std::move(task));
        }
        LOG_INFO() << "Pool initialization is ongoing";
        StartMaintainTask();
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
    StartMaintainTask();
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
                userver::engine::SemaphoreLock{_size_semaphore, kConnectingTimeout};
        }
        return DoConnect(std::move(size_lock), std::move(conn_settings));
    });
}
bool ConnectionPool::DoConnect(
    userver::engine::SemaphoreLock size_lock, ConnectionSettings&& conn_settings
) {
    if (!size_lock) return false;
    LOG_TRACE() << "Creating Cassandra connection, current pool size: "
                << _size_semaphore.UsedApprox();

    const userver::engine::SemaphoreLock connecting_lock{
        _connecting_semaphore, kConnectingTimeout
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
    if (conn->IsExpired() && conn->IsIdle()) {
        DropExpiredConnection(conn);
        return;
    } else if (conn->IsBroken()) {
        DropBrokenConnection(conn);
        return;
    }

    LOG_DEBUG("PUSH TO QUEUE");
    auto locked_connections = _connections.Lock();
    locked_connections->push_back(conn);

    LOG_DEBUG("SUCCESS PUSH TO QUEUE");
}

constexpr std::chrono::seconds kRecentErrorPeriod{15};
constexpr auto kPendingConnectsMax{1};
void ConnectionPool::TryCreateConnectionAsync() {
    auto conn_settings = _connection_settings.ReadCopy();
    if (_recent_conn_errors.GetStatsForPeriod(kRecentErrorPeriod, true) <
        conn_settings.recent_errors_threshold) {
        userver::engine::SemaphoreLock size_lock{_size_semaphore, std::try_to_lock};
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

ConnectionPool::~ConnectionPool() {
    _maintain_task.Stop();
    _connect_task_storage.CancelAndWait();
    Clear();
}

void ConnectionPool::Clear() {
    {
        auto locked = _connections.Lock();
        for (auto* conn : locked.GetUnsafeForStableSubobject()) {
            DeleteConnection(conn);
        }
        locked->clear();
    }
    _close_task_storage.CancelAndWait();
}

[[nodiscard]] Connection* ConnectionPool::Acquire(userver::engine::Deadline deadline
) {
    if (deadline.IsReached()) {
        return nullptr;
    }
    auto locked = _connections.Lock();

    // Remove broken and expired+idle connections
    for (auto it = locked->begin(); it != locked->end();) {
        Connection* c = *it;
        if (c->IsBroken()) {
            DropBrokenConnection(c);
            it = locked->erase(it);
        } else if (c->IsExpired() && c->IsIdle()) {
            DropExpiredConnection(c);
            it = locked->erase(it);
        } else {
            ++it;
        }
    }

    // Select least-loaded connection eligible for new requests.
    // Expired+busy connections stay in the vector until their streams
    // finish, but must not receive new requests.
    Connection* conn = nullptr;
    for (auto* c : *locked) {
        if (c->IsExpired()) continue;
        if (conn == nullptr || c->GetUsedStreams() < conn->GetUsedStreams()) {
            conn = c;
        }
    }

    return conn;
}

ResultSet ConnectionPool::Execute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    auto conn = Acquire(userver::engine::Deadline{});

    if (CanPrepareStatement(statement_cmd_ctl, _prepared_statements_cache_enabled)) {
        if (statement_cmd_ctl.has_value()) {
            LOG_DEBUG(
                "PREPARED OVERRIDE: {}",
                static_cast<int>(statement_cmd_ctl->prepared_statements_enabled)
            );
        }
        LOG_DEBUG("TRYING TO GET PREPARED");
        auto prepared_id_ptr =
            _prepared_statements_map.Get(query.GetStatement().GetUnderlying());
        if (prepared_id_ptr) {
            LOG_DEBUG("FOUND PREPARED");
            // here we may throws unprepared exception and we need to prepare it same
            // way as on wrong branch of current if-clause
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

    if (CanPrepareStatement(statement_cmd_ctl, _prepared_statements_cache_enabled)) {
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

void ConnectionPool::StartMaintainTask() {
    using Flags = userver::utils::PeriodicTask::Flags;
    _maintain_task.Start(
        "maintain_task", {kMaintainInterval, Flags::kStrong}, [this] { Maintain(); }
    );
}

const Query kPing{"SELECT cluster_name FROM system.local"};
constexpr auto kIdleDropLimit = 1;

constexpr StaticQueryParameters<0> kNoParams;

void ConnectionPool::Release(Connection* connection) {
    if (connection->IsExpired() && connection->IsIdle()) {
        DropExpiredConnection(connection);
    } else if (connection->IsBroken()) {
        DropBrokenConnection(connection);
    } else {
        Push(connection);
    }
}

void ConnectionPool::Maintain() {
    if (_wait_count > 0) {
        LOG_DEBUG() << "No ping required for connection pool to node: "
                    << _description.contact_point.GetUnderlying();
        return;
    }

    LOG_DEBUG() << "Ping connection pool "
                << _description.contact_point.GetUnderlying();

    // Берём размер из вектора, а не из семафора:
    // семафор освобождается асинхронно (при завершении close-таски),
    // поэтому UsedApprox() может включать соединения в процессе закрытия
    // и завышать реальный размер пула.
    size_t pool_size;
    {
        auto locked = _connections.Lock();
        pool_size = locked->size();
    }

    auto drop_left = kIdleDropLimit;
    auto settings = _settings.Read();

    while (pool_size > 0) {
        try {
            auto deleter = [this](Connection* c) { DeleteConnection(c); };
            std::unique_ptr<Connection, decltype(deleter)> conn(
                AcquireImmediate(), deleter
            );
            if (!conn) {
                LOG_DEBUG() << "All connections to `"
                            << _description.contact_point.GetUnderlying()
                            << "` are busy";
                break;
            }

            // Добавлено conn->IsIdle(): нельзя закрывать соединение,
            // у которого есть активные стримы — это оборвёт pending-запросы.
            // Если соединение занято, оно попадёт в ping-ветку и
            // вернётся в пул через Release().
            if (pool_size > settings->min_size && drop_left > 0 && conn->IsIdle()) {
                --drop_left;
                LOG_DEBUG() << "Drop idle connection to `"
                            << _description.contact_point.GetUnderlying() << '`';
                conn->Close();
            } else {
                const auto releaser = [this](Connection* c) { Release(c); };
                std::unique_ptr<Connection, decltype(releaser)> capture(
                    conn.release(), releaser
                );
                auto prepared_id_ptr =
                    _prepared_statements_map.Get(kPing.GetStatement().GetUnderlying()
                    );
                if (!prepared_id_ptr) {
                    auto prepared_id = capture->Prepare(kPing.GetStatement());
                    _prepared_statements_map.Put(
                        kPing.GetStatement().GetUnderlying(), prepared_id
                    );
                    capture->ExecutePrepared(
                        Consistency::kLocalOne,
                        prepared_id,
                        QueryParameters{kNoParams},
                        std::nullopt
                    );
                } else {
                    try {
                        capture->ExecutePrepared(
                            Consistency::kLocalOne,
                            *prepared_id_ptr,
                            QueryParameters{kNoParams},
                            std::nullopt
                        );
                    } catch (const exceptions::Unprepared& e) {
                        LOG_LIMITED_WARNING(
                            "Prepared statements were discarded on node: {}",
                            _description.contact_point.GetUnderlying()
                        );
                        _prepared_statements_map.Invalidate();
                    }
                }
            }
        } catch (const exceptions::RuntimeError& e) {
            LOG_LIMITED_WARNING()
                << "Exception while pinging connection to `"
                << _description.contact_point.GetUnderlying() << "`: " << e;
        }
        --pool_size;
    }
}

Connection* ConnectionPool::AcquireImmediate() {
    Connection* conn = nullptr;
    auto locked_connections = _connections.Lock();
    auto it = locked_connections->begin();
    while (it != locked_connections->end()) {
        conn = *it;
        if (conn->IsExpired()) {
            if (conn->IsIdle()) {
                DropExpiredConnection(conn);
                it = locked_connections->erase(it);
            } else {
                ++it;
            }
            continue;
        }
        if (conn->IsBroken()) {
            DropBrokenConnection(conn);
            it = locked_connections->erase(it);
            continue;
        }

        locked_connections->erase(it);
        return conn;
    }
    return nullptr;
}

void ConnectionPool::DeleteConnection(Connection* connection) {
    // stats incrementing
    delete connection;
}

void ConnectionPool::DropBrokenConnection(Connection* connection) {
    LOG_WARNING("Released connection in closed state. Deleting...");
    DeleteConnection(connection);
}

void ConnectionPool::DropExpiredConnection(Connection* connection) {
    LOG_INFO("Dropping expired connection");
    DeleteConnection(connection);
}

}  // namespace cassandra::detail
