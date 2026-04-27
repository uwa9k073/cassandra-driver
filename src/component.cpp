#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/component.hpp>
#include <cassandra/database.hpp>
#include <cassandra/database_fwd.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <cassandra/secdist.hpp>
#include <cassandra/session.hpp>
#include <memory>
#include <string_view>
#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/zstring_view.hpp>
#include <userver/yaml_config/fwd.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>
#include <cassandra_configs.hpp>

namespace components {

namespace {
const std::string kSimpleStaticConfigSchema = R"(
            type: object
            description: Apache Cassandra client component
            additionalProperties: false
            properties:
                keyspace:
                    type: string
                    description: keyspace name
        )";
const std::string kFullStaticConfigSchema = R"(
type: object
description: Apache Cassandra client component
additionalProperties: false
properties:
    keyspace:
        type: string
        description: keyspace name
    blocking_task_processor:
        type: string
        description: name of task processor for background blocking operations
        defaultDescription: engine::current_task::GetBlockingTaskProcessor()
    min-pool-size:
        type: integer
        description: |
            number of connections created initially by this component
            instance to each of the provided PostgreSQL hosts. Connections
            are kept even without requests
        defaultDescription: 4
    max-pool-size:
        type: integer
        description: |
            maximum number of connections that can be created by this
            component instance to each of the provided PostgreSQL hosts for
            "connlimit_mode: manual". Should not be less than `min_pool_size`
        defaultDescription: 15
    sync-start:
        type: boolean
        description: perform initial connections synchronously
        defaultDescription: false
    dns_resolver:
        type: string
        description: server hostname resolver type (getaddrinfo or async)
        defaultDescription: 'async'
        enum:
          - getaddrinfo
          - async
    persistent-prepared-statements:
        type: boolean
        description: cache prepared statements or not
        defaultDescription: true
    max-ttl-sec:
        type: integer
        minimum: 1
        description: the maximum lifetime for connections
    prepared-statement-cache-ways:
        type: integer
        description: prepared statements cache ways via NWayLRU
        defaultDescription: 16
    prepared-statement-cache-way-size:
        type: integer
        description: prepared statements cache way size via NWayLRU
        defaultDescription: 200
    max-queue-size:
        type: integer
        description: |
            maximum number of clients waiting for a connection.
            storages::postgres::PoolError is thrown if a new request exceeds
            this limit
        defaultDescription: 200
)";
}  // namespace
Cassandra::Cassandra(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
)
    : userver::components::ComponentBase(config, context),
      _database(std::make_shared<cassandra::Database>()) {
    auto* resolver = userver::clients::dns::GetResolverPtr(config, context);

    std::string keyspace = config["keyspace"].As<std::string>();
    auto& secdist = context.FindComponent<userver::components::Secdist>();
    auto cluster_desc = secdist.Get()
                            .Get<cassandra::CassandraSecdist>()
                            .GetShardedClusterDescription(keyspace);

    const auto task_processor_name =
        config["blocking_task_processor"].As<std::optional<std::string>>();
    auto& bg_task_processor =
        task_processor_name
            ? context.GetTaskProcessor(*task_processor_name)
            : userver::engine::current_task::GetBlockingTaskProcessor();

    auto metrics = context.FindComponent<userver::components::StatisticsStorage>()
                       .GetMetricsStorage();
    
    auto pool_settings = config.As<cassandra::PoolSettings>();
    auto connection_settings = config.As<cassandra::ConnectionSettings>();

    cassandra::SessionSettings session_settings{
        .keyspace_name = keyspace, .pool_settings = pool_settings, .connection_settings = connection_settings
    };
    _database->_session = std::make_shared<cassandra::Session>(
        cluster_desc, resolver, bg_task_processor, session_settings, metrics
    );
    LOG_DEBUG("Component ready");
}
userver::yaml_config::Schema Cassandra::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
        kFullStaticConfigSchema
    );
}

cassandra::SessionPtr Cassandra::GetSessionPtr() const {
    return _database->GetSessionPtr();
}
}  // namespace components
