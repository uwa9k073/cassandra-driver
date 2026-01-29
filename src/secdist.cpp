#include <cassandra/secdist.hpp>
#include <string_view>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <vector>
#include "cassandra/node_description.hpp"

namespace cassandra {
CassandraSecdist::CassandraSecdist(const userver::formats::json::Value& doc) {
  userver::storages::secdist::CheckIsObject(doc, "cassandra_settings");
  const auto& cassandra_settings = doc["cassandra_settings"];
  if (!cassandra_settings.IsObject()) {
    throw userver::storages::secdist::SecdistError(
        "'cassandra_settings' secdist section is wrong format");
  }

  const auto& keyspaces = cassandra_settings["keyspaces"];

  if (!keyspaces.IsObject()) {
    throw userver::storages::secdist::SecdistError(
        "'keyspaces' secdist is in wrong format");
  }

  for (auto it = keyspaces.begin(); it != keyspaces.end(); ++it) {
    const std::string& keyspace_name = it.GetName();
    const auto& nodes = (*it)["nodes"];
    userver::storages::secdist::CheckIsArray(nodes, keyspace_name);

    auto& sharded_cluster_for_db = _sharded_cluster_descs[keyspace_name];

    sharded_cluster_for_db.reserve(nodes.GetSize());

    for (auto shard_it = nodes.begin(); shard_it != nodes.end(); ++shard_it) {
      const auto& shard = *shard_it;
      userver::storages::secdist::CheckIsObject(
          shard, fmt::format("shard {} description for keyspace '{}'",
                             shard_it.GetIndex(), keyspace_name));

      sharded_cluster_for_db.push_back((*shard_it).As<NodeDescription>());
    }
  }
}

std::vector<NodeDescription> CassandraSecdist::GetShardedClusterDescription(
    const std::string& keyspace_name) const {
  const auto it = _sharded_cluster_descs.find(keyspace_name);
  if (it == _sharded_cluster_descs.end()) {
    throw userver::storages::secdist::SecdistError(fmt::format(
        "No cassandra secdist settings for keyspace '{}'", keyspace_name));
  }
  return it->second;
}

}  // namespace cassandra
