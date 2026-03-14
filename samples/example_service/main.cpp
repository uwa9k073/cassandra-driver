#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utils/daemon_run.hpp>

#include <cassandra/session.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include <cassandra/component.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/row_types.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/server/http/http_status.hpp>

namespace views {
class Cassandra final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "cassandra-view";
    Cassandra(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const Value& request_json,
        RequestContext& context
    ) const override;

private:
    cassandra::SessionPtr _session_ptr;
};
}  // namespace views

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::components::HttpClient>()
                              .Append<userver::clients::dns::Component>()
                              .Append<userver::components::Secdist>()
                              .Append<userver::components::DefaultSecdistProvider>()
                              .Append<userver::server::handlers::TestsControl>()
                              .Append<userver::congestion_control::Component>()
                              .Append<components::Cassandra>("cassandra-component")
                              .Append<views::Cassandra>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}

namespace views {

const ::cassandra::Query kBasicSelect{"select cluster_name from system.local"};

Cassandra::Cassandra(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::server::handlers::HttpHandlerJsonBase(config, context),
      _session_ptr(context
                       .FindComponent<::components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

userver::formats::json::Value Cassandra::HandleRequestJsonThrow(
    const HttpRequest& request,
    const Value& /*request_json*/,
    RequestContext& /*context*/
) const {
    if (_session_ptr) {
        auto result =
            _session_ptr->Execute(cassandra::Consistency::kLocalOne, kBasicSelect);

        if (!result.RowsAffected()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::NotFound);
            return {};
        }

        LOG_DEBUG(
            "RowsAffected={}, ColumnsAffected={}",
            result.RowsAffected(),
            result.ColumnsAffected()
        );
        auto cassandra_cluster_name =
            result.AsSingleRow<cassandra::io::LongString>(cassandra::io::kFieldTag);
        LOG_DEBUG("clusterName={}", cassandra_cluster_name.GetUnderlying());
        std::string underlying_cluster_name = cassandra_cluster_name.GetUnderlying();

        return userver::formats::json::MakeObject(
            "clusterName", underlying_cluster_name
        );
    } else {
        request.SetResponseStatus(
            userver::server::http::HttpStatus::InternalServerError
        );
        return userver::formats::json::MakeObject(
            "message", "session_ptr is nullptr"
        );
    }
}

}  // namespace views
