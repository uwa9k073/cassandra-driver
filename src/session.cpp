#include <cassandra/session.hpp>
#include <memory>
#include <vector>
#include "session_impl.hpp"
#include <userver/utils/statistics/metrics_storage.hpp>

namespace cassandra {
Session::Session(std::vector<NodeDescription> node_description,
                 userver::clients::dns::Resolver* resolver,
                 userver::engine::TaskProcessor& task_processor,
                 userver::utils::statistics::MetricsStoragePtr metrics_storage) {
  _pimpl = std::make_unique<detail::SessionImpl>(std::move(node_description),
                                                 resolver, task_processor,
                                                 std::move(metrics_storage));
}
}  // namespace cassandra
