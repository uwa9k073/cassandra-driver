#pragma once

#include <cassandra/options.hpp>
#include <cassandra/session.hpp>
#include <memory>
#include <userver/rcu/rcu.hpp>
#include "detail/connection_pool.hpp"

namespace cassandra::detail {
class SessionImpl {
public:
    SessionImpl(
        std::vector<NodeDescription> node_description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& task_processor,
        SessionSettings session_settings,
        userver::utils::statistics::MetricsStoragePtr metrics_storage
    );

private:
    void CreateTopology(std::vector<NodeDescription> node_description);
    userver::clients::dns::Resolver* resolver_{};
    userver::engine::TaskProcessor& bg_task_processor_;
    userver::rcu::Variable<SessionSettings> _session_settings;
    // userver::dynamic_config::Source config_source_;
    // DefaultCommandControls default_cmd_ctls_;
    // const error_injection::Settings ei_settings_;
    USERVER_NAMESPACE::utils::statistics::MetricsStoragePtr metrics_;

    std::vector<std::shared_ptr<ConnectionPool>> _pools;
};
}  // namespace cassandra::detail
