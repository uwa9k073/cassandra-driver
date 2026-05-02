#include <cassandra_configs.hpp>

namespace cassandra {

ConnectionSettings
Parse(const userver::yaml_config::YamlConfig& config, userver::formats::parse::To<ConnectionSettings>) {
    return {
        .max_ttl = std::chrono::seconds(config["max-ttl-sec"].As<int>(30)),
        .recent_errors_threshold =
            config["recent-errors-threshold"].As<std::size_t>(2),
    };
}

PoolSettings
Parse(const userver::yaml_config::YamlConfig& config, userver::formats::parse::To<PoolSettings>) {
    return {
        .min_size = config["min-pool-size"].As<std::size_t>(4),
        .max_size = config["max-pool-size"].As<std::size_t>(15),
        .max_queue_size = config["max-queue-size"].As<std::size_t>(200),
        // .connecting_limit = config["connecting-limit"].As<std::size_t>(0),
        .prepared_statement_cache_ways =
            config["prepared-statement-cache-ways"].As<std::size_t>(16),
        .prepared_statement_cache_way_size =
            config["prepared-statement-cache-way-size"].As<std::size_t>(200),
        .prepared_statement_cache_enabled =
            config["persistentprepared-statement"].As<bool>(true),
    };
}
}  // namespace cassandra
