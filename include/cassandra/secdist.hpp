#pragma once

#include <cassandra/node_description.hpp>
#include <string>
#include <unordered_map>
#include <userver/formats/json/value.hpp>
#include <userver/utils/zstring_view.hpp>
#include <vector>
namespace cassandra {
struct CassandraSecdist {
    explicit CassandraSecdist(const userver::formats::json::Value& doc);

    std::vector<NodeDescription> GetShardedClusterDescription(
        const std::string& keyspace
    ) const;

private:
    std::unordered_map<std::string, std::vector<NodeDescription>>
        _sharded_cluster_descs;
};
}  // namespace cassandra
