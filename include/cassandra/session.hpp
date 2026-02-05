#pragma once
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <memory>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>

namespace cassandra {
namespace detail {
class SessionImpl;
using SessionImplPtr = std::unique_ptr<SessionImpl>;
}  // namespace detail
class Session {
public:
    Session(
        std::vector<NodeDescription> node_description,
        userver::clients::dns::Resolver* resolver,
        userver::engine::TaskProcessor& task_processor,
        SessionSettings session_settings,
        userver::utils::statistics::MetricsStoragePtr metrics_storage
    );

    ~Session();

private:
    detail::SessionImplPtr _pimpl;
};
}  // namespace cassandra
