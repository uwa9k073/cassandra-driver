#include <cassandra/secdist.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/helpers.hpp>

namespace cassandra {
CassandraSecdist::CassandraSecdist(const userver::formats::json::Value& doc) {
  const auto& cassandra_settings = doc["cassandra_settings"];
  if (cassandra_settings.IsMissing()) {
    LOG_WARNING("'cassandra_settings' secdist section is empty");
    return;
  }

  userver::storages::secdist::CheckIsObject(cassandra_settings,
                                            "cassandra_settings");
  const auto& nodes = cassandra_settings["nodes"];
  userver::storages::secdist::CheckIsObject(nodes, "nodes");
}
}  // namespace cassandra
