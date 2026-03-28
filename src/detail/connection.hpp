#pragma once

#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <detail/connection_impl.hpp>
#include <memory>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/crypto/openssl.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include "cassandra/batch_query.hpp"

namespace cassandra::detail {

class ConnectionImpl;

class Connection {
public:
    static std::unique_ptr<Connection> Connect(
        NodeDescription description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& bg_task_processor,
        userver::concurrent::BackgroundTaskStorageCore& bg_task_storage,
        ConnectionSettings settings,
        userver::engine::SemaphoreLock&& size_lock,
        userver::utils::statistics::MetricsStoragePtr metric
    );

    ~Connection();

    bool IsExpired() const;

    bool IsBroken() const;

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

    ResultSet BatchExecute(
        Consistency level,
        std::vector<BatchStatement>&& batch_statements,
        OptionalCommandControl statement_cmd_ctl
    );

private:
    Connection();
    std::unique_ptr<ConnectionImpl> _pimpl;
};
}  // namespace cassandra::detail
