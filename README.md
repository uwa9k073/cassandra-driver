# userver-cql-driver

Драйвер Apache Cassandra (Native Protocol V4) основанный на фреймворке Userver.



 Пример взаимодействия с драйвером:
 ```cpp
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

Cassandra::Cassandra(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::server::handlers::HttpHandlerJsonBase(config, context),
      _session_ptr(context
                       .FindComponent<::components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

const ::cassandra::Query kBasicSelect{"select cluster_name from system.local"};
const ::cassandra::Query kBenchInsertQuery{
    "insert into benchmark_ks.my_table (id, name) values (?, ?)"
};
const ::cassandra::Query kBenchSelectQuery{
    "select id, name from benchmark_ks.my_table"
};

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
        auto id = request_json["id"].As<int>();
        auto name = request_json["name"].As<std::string>();

        auto result = _session_ptr->Execute(
            cassandra::Consistency::kLocalOne, kBenchInsertQuery, id, name
        );

        auto select_result = _session_ptr->Execute(
            cassandra::Consistency::kLocalOne, kBenchSelectQuery
        );
        if (!select_result.RowsAffected()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::NotFound);
            return {};
        }

        LOG_DEBUG(
            "RowsAffected={}, ColumnsAffected={}",
            select_result.RowsAffected(),
            select_result.ColumnsAffected()
        );
        auto cassandra_row =
            select_result.AsSingleRow<MyRow>(cassandra::io::kRowTag);

        return userver::formats::json::MakeObject(
            "id", cassandra_row.id, "name", cassandra_row.name
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
 ```