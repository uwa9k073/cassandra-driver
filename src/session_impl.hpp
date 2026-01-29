#pragma once

#include <cassandra/session.hpp>

namespace cassandra::detail {
class SessionImpl {
 public:
  SessionImpl(std::vector<NodeDescription> node_description,
              userver::clients::dns::Resolver* resolver,
              userver::engine::TaskProcessor& task_processor,
              userver::utils::statistics::MetricsStoragePtr metrics_storage);

 private:
  userver::clients::dns::Resolver* resolver_{};
  userver::engine::TaskProcessor& bg_task_processor_;
  // userver::dynamic_config::Source config_source_;
  // DefaultCommandControls default_cmd_ctls_;
  // const error_injection::Settings ei_settings_;
  USERVER_NAMESPACE::utils::statistics::MetricsStoragePtr metrics_;
};
}  // namespace cassandra::detail
