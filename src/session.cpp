#include <cassandra/session.hpp>
#include <memory>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <vector>
#include "cassandra/options.hpp"
#include "session_impl.hpp"

namespace cassandra {
Session::Session(
    std::vector<NodeDescription> node_description,
    userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& task_processor,
    SessionSettings session_settings,
    userver::utils::statistics::MetricsStoragePtr metrics_storage
) {
    _pimpl = std::make_unique<detail::SessionImpl>(
        std::move(node_description), resolver, task_processor, session_settings, std::move(metrics_storage)
    );
}

Session::~Session() = default;
}  // namespace cassandra
