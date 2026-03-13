#include "session_impl.hpp"
#include "cassandra/exception.hpp"
#include "cassandra/options.hpp"
#include "detail/connection_pool.hpp"

namespace cassandra::detail {

SessionImpl::SessionImpl(
    std::vector<NodeDescription> node_description,
    userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& task_processor,
    SessionSettings session_settings,
    userver::utils::statistics::MetricsStoragePtr metrics_storage
)
    : resolver_(resolver),
      bg_task_processor_(task_processor),
      _session_settings(session_settings),
      metrics_(std::move(metrics_storage)) {
    CreateTopology(node_description);
}

void SessionImpl::CreateTopology(std::vector<NodeDescription> node_description) {
    if (node_description.empty()) {
        throw exceptions::SessionError{
            "Cannont create session from an empty node list"
        };
    }

    auto session_settings = _session_settings.Read();

    LOG_DEBUG("Starting pools initialization");

    for (const auto& node : node_description) {
        auto pool = ConnectionPool::Create(
            node,
            resolver_,
            bg_task_processor_,
            session_settings->keyspace_name,
            InitMode::kAsync,
            session_settings->pool_settings,
            session_settings->connection_settings,
            metrics_
        );

        _pools.emplace_back(pool);
    }
    LOG_DEBUG("Pool initialize");
}
}  // namespace cassandra::detail
