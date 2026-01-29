#include "session_impl.hpp"

namespace cassandra::detail {

SessionImpl::SessionImpl(
    std::vector<NodeDescription> node_description, userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& task_processor,
    userver::utils::statistics::MetricsStoragePtr metrics_storage)
    : resolver_(resolver),
      bg_task_processor_(task_processor),
      metrics_(std::move(metrics_storage)) {
  // Constructor implementation goes here
}
}  // namespace cassandra::detail
