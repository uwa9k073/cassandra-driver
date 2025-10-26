#include <cassandra/component.hpp>
#include <cassandra/node_description.hpp>
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
#include <userver/testsuite/tasks.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/yaml_config/fwd.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

namespace components {
Cassandra::Cassandra(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context) {
  auto* resolver = userver::clients::dns::GetResolverPtr(config, context);

  const auto& nodes_description =
      config["nodes"].As<std::vector<cassandra::NodeDescription>>();

  for (const auto& node_description : nodes_description) {
    LOG_DEBUG("CASSANDRA NODE INIT");
  }
}

userver::yaml_config::Schema Cassandra::GetStaticConfigSchema() {
  return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(
      R"(
type: object
description: Apache Cassandra client component
additionalProperties: false
properties:
    nodes:
        type: array
        items:
            type: object
            additionalProperties: false
            properties:
                use-ssl:
                    type: boolean
                    description: use SSl for connect to node
                use-compression:
                    type: boolean
                    description: use lz4 compression for read/write frames
                allow-all:
                    type: boolean
                    description: allow all authentifiactor
                    defaultDescription: true
                auth:
                    type: object
                    additionProperties: false
                    properties:
                        username:
                            type: string
                        password:
                            type: string
                    defaultDescription: null
                contact-point:
                    type: string
                    description: IPV4 address
                port:
                    type: integer
                    description: Node port
                    defaultDescription: 9042
    default-keyspace:
        type: string
        description: default keyspace name
    default-consistency-level:
        type: string
        description: default consistency level
    blocking_task_processor:
        type: string
        description: name of task processor for background blocking operations
        defaultDescription: engine::current_task::GetBlockingTaskProcessor()
    min_pool_size:
        type: integer
        description: |
            number of connections created initially by this component instance to each of the provided PostgreSQL
            hosts. Connections are kept even without requests
        defaultDescription: 4
    max_pool_size:
        type: integer
        description: |
            maximum number of connections that can be created by this component instance to each of the provided
            PostgreSQL hosts for "connlimit_mode: manual". Should not be less than `min_pool_size`
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
    user-types-enabled:
        type: boolean
        description: disabling will disallow use of user-defined types
        defaultDescription: true
    ignore_unused_query_params:
        type: boolean
        description: disable check for not-NULL query params that are not used in query
        defaultDescription: false
    max-ttl-sec:
        type: integer
        minimum: 1
        description: the maximum lifetime for connections
    discard-all-on-connect:
        type: boolean
        description: execute discard all on new connections
        defaultDescription: true
    deadline-propagation-enabled:
        type: boolean
        description: whether statement timeout is affected by deadline propagation
        defaultDescription: true
    monitoring-dbalias:
        type: string
        description: name of the database for monitorings
        defaultDescription: calculated from dbalias or dbconnection options
    max_prepared_cache_size:
        type: integer
        description: prepared statements cache size limit
        defaultDescription: 200
    max_queue_size:
        type: integer
        description: |
            maximum number of clients waiting for a connection. storages::postgres::PoolError is thrown if a new
            request exceeds this limit
        defaultDescription: 200
)");
}
}  // namespace components
