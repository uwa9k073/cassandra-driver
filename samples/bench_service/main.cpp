#include <chrono>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/formats/json/value_builder.hpp>
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
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/log_level.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/datetime_light.hpp>
#include <vector>

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
                              .Append<userver::server::handlers::LogLevel>()
                              .Append<components::Cassandra>("cassandra-component")
                              .Append<views::Cassandra>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}

namespace views {

Cassandra::Cassandra(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::server::handlers::HttpHandlerJsonBase(config, context),
      _session_ptr(context
                       .FindComponent<::components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

const ::cassandra::Query kInsertQuery{
    "insert into benchmark_ks.my_table (id, name) values (?, ?)"
};
const ::cassandra::Query kSelectQuery{"select id, name from benchmark_ks.my_table"};

const ::cassandra::Query kTruncQuery{"TRUNCATE TABLE benchmark_ks.my_table"};

struct MyRow {
    cassandra::io::Int id;
    std::string name;
};

userver::formats::json::Value Cassandra::HandleRequestJsonThrow(
    const HttpRequest& request,
    const Value& request_json,
    RequestContext& /*context*/
) const {
    if (_session_ptr) {
        auto turn_count = request_json["turnCount"].As<int>();
        auto loop_count = request_json["loopCount"].As<int>();
        std::string data = "name";

        std::vector<int> results;

        for (auto _ = 0; _ < turn_count; ++_) {
            _session_ptr->Execute(cassandra::Consistency::kLocalOne, kTruncQuery);
            auto start = userver::utils::datetime::Now();
            for (int id = 0; id < loop_count; ++id) {
                _session_ptr->Execute(
                    cassandra::Consistency::kLocalOne,
                    ::cassandra::CommandControl{
                        std::chrono::seconds{10},
                        std::chrono::seconds{10},
                        cassandra::CommandControl::PreparedStatementsOptionOverride::
                            kDisabled
                    },
                    kInsertQuery,
                    id,
                    data
                );
            }
            auto end = userver::utils::datetime::Now();
            results.push_back(
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                    .count()
            );
        }

        userver::formats::json::ValueBuilder builder;
        builder["durations"] = results;

        return builder.ExtractValue();
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
