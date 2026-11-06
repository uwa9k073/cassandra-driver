#include <detail/routing/round_robin_routing.hpp>
#include "detail/connection_pool.hpp"

namespace cassandra::detail::routing {

RoundRobinPolicy::RoundRobinPolicy(
    std::span<NodeDescription> node_description,
    userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& bg_task_processor,
    const std::string& keyspace,
    InitMode init_mode,
    PoolSettings settings,
    ConnectionSettings connection_settings,
    userver::utils::statistics::MetricsStoragePtr metrics
)
    : PolicyBase() {
    for (const auto& node : node_description) {
        auto pool = ConnectionPool::Create(
            node,
            resolver,
            bg_task_processor,
            keyspace,
            init_mode,
            settings,
            connection_settings,
            metrics
        );

        _pools.emplace_back(pool);
    }
}

std::shared_ptr<ConnectionPool> RoundRobinPolicy::FindPool() {
    auto n = _pools.size();
    if (n == 0) return nullptr;

    // Пул выбирается циклически
    auto index = _index.fetch_add(1, std::memory_order_relaxed) % n;
    LOG_DEBUG("NODE INDEX: {}", index);
    return _pools[index % n];
}
}  // namespace cassandra::detail::routing
