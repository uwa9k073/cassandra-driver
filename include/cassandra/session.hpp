#pragma once
#include <cassandra/batch_query.hpp>
#include <cassandra/detail/query_parameters.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <memory>
#include <optional>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/zstring_view.hpp>

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

    template <typename... Args>
    ResultSet Execute(
        Consistency level, userver::utils::zstring_view query, Args&&... args
    );

    template <typename... Args>
    ResultSet Execute(Consistency level, const Query& query, Args&&... args) {
        return Execute(level, std::nullopt, query, std::forward<Args>(args)...);
    }

    template <typename... Args>
    ResultSet Execute(
        Consistency level,
        CommandControl statement_cmd_ctl,
        const Query& query,
        Args&&... args
    ) {
        LOG_DEBUG(
            "EXECUTE WITH CTL: {}",
            static_cast<int>(statement_cmd_ctl.prepared_statements_enabled)
        );
        return Execute(
            level,
            OptionalCommandControl{statement_cmd_ctl},
            query,
            std::forward<Args>(args)...
        );
    }

    ResultSet BatchExecute(const BatchQueryStore& store);

    ResultSet BatchExecute(
        const BatchQueryStore& store, CommandControl statement_cmd_ctl
    );

    template <typename... Args>
    ResultSet Execute(
        Consistency level,
        OptionalCommandControl statement_cmd_ctl,
        const Query& query,
        const Args&... args
    ) {
        detail::StaticQueryParameters<sizeof...(args)> params;
        params.Write(args...);
        return DoExecute(level, query, QueryParameters{params}, statement_cmd_ctl);
    }

private:
    detail::SessionImplPtr _pimpl;
    ResultSet DoExecute(
        Consistency level,
        const Query& query,
        const QueryParameters& params,
        OptionalCommandControl statement_cmd_ctl
    );
};
}  // namespace cassandra
