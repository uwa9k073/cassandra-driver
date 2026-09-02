#include <cassandra/exception.hpp>
#include <cassandra/options.hpp>
#include <detail/connection_pool.hpp>
#include <detail/routing/round_robin_routing.hpp>
#include <memory>
#include <session_impl.hpp>
#include "cassandra/exception.hpp"

namespace cassandra::detail {

SessionImpl::SessionImpl(
    std::span<NodeDescription> node_description,
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

void SessionImpl::CreateTopology(std::span<NodeDescription> node_description) {
    if (node_description.empty()) {
        throw exceptions::SessionError{
            "Cannont create session from an empty node list"
        };
    }

    auto session_settings = _session_settings.Read();

    LOG_DEBUG("Starting pools initialization");

    if (session_settings->load_balancing_policy == "RoundRobin") {
        this->_routing_policy = std::make_unique<routing::RoundRobinPolicy>(
            node_description,
            resolver_,
            bg_task_processor_,
            session_settings->keyspace_name,
            InitMode::kAsync,
            session_settings->pool_settings,
            session_settings->connection_settings,
            metrics_
        );
    } else {
        throw exceptions::SessionError("invalid load_balancing_policy");
    }
    LOG_DEBUG("Pool initialize");
}

std::shared_ptr<ConnectionPool> SessionImpl::FindPool() {
    return _routing_policy->FindPool();
}

ResultSet SessionImpl::Execute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl

) {
    return FindPool()->Execute(level, query, params, statement_cmd_ctl);
}

ResultSet SessionImpl::BatchExecute(
    const BatchQueryStore& store, OptionalCommandControl statement_cmd_ctl
) {
    return FindPool()->BatchExecute(store, statement_cmd_ctl);
}

}  // namespace cassandra::detail
