#include <cassandra/session.hpp>
#include <memory>
#include <optional>
#include <userver/utils/statistics/metrics_storage.hpp>
#include <vector>
#include "cassandra/batch_query.hpp"
#include "cassandra/options.hpp"
#include "cassandra/result_set.hpp"
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
        std::move(node_description),
        resolver,
        task_processor,
        session_settings,
        std::move(metrics_storage)
    );
}

Session::~Session() = default;

ResultSet Session::DoExecute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    return _pimpl->Execute(level, query, params, statement_cmd_ctl);
}

ResultSet Session::BatchExecute(const BatchQueryStore& batch_store) {
    return _pimpl->BatchExecute(batch_store, std::nullopt);
}

ResultSet Session::BatchExecute(
    const BatchQueryStore& batch_store, CommandControl statement_cmd_ctl
) {
    return _pimpl->BatchExecute(
        batch_store, OptionalCommandControl{statement_cmd_ctl}
    );
}
}  // namespace cassandra
