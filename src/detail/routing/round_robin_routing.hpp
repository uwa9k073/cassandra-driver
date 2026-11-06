#pragma once

#include <atomic>
#include <cstddef>
#include <detail/routing/routing_base.hpp>

namespace cassandra::detail::routing {

class RoundRobinPolicy : public PolicyBase {
public:
    RoundRobinPolicy(
        std::span<NodeDescription> node_description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& bg_task_processor,
        const std::string& keyspace,
        InitMode init_mode,
        PoolSettings settings,
        ConnectionSettings connection_settings,
        userver::utils::statistics::MetricsStoragePtr metrics
    );
    std::shared_ptr<ConnectionPool> FindPool() override;

private:
    std::atomic<std::size_t> _index{0};
    std::vector<std::shared_ptr<ConnectionPool>> _pools;
};

}  // namespace cassandra::detail::routing
