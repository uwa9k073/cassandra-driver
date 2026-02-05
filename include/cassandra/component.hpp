#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/database_fwd.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

namespace components {
class Cassandra final : public userver::components::ComponentBase {
public:
    Cassandra(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

    cassandra::SessionPtr GetSessionPtr() const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    cassandra::DatabasePtr _database;
};
}  // namespace components
