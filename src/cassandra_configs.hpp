#include <cassandra/options.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/yaml_config/yaml_config.hpp>
namespace cassandra {
ConnectionSettings
Parse(const userver::yaml_config::YamlConfig& config, userver::formats::parse::To<ConnectionSettings>);
PoolSettings
Parse(const userver::yaml_config::YamlConfig& config, userver::formats::parse::To<PoolSettings>);
}  // namespace cassandra
