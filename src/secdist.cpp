#include <cassandra/secdist.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/storages/secdist/helpers.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <vector>
#include "cassandra/node_description.hpp"

namespace cassandra {
CassandraSecdist::CassandraSecdist(const userver::formats::json::Value& doc) {
  userver::storages::secdist::CheckIsObject(doc,
                                            "cassandra_settings");
  const auto& cassandra_settings = doc["cassandra_settings"];
  if (cassandra_settings.IsMissing()) {
    throw userver::storages::secdist::SecdistError("'cassandra_settings' secdist section is empty");
  }
  
  if(!cassandra_settings.IsObject()) {
    throw userver::storages::secdist::SecdistError("'cassandra_settings' secdist is in wrong format");
  }
  
  const auto& nodes = cassandra_settings["nodes"];
  
  if(nodes.IsMissing()){
      throw userver::storages::secdist::SecdistError("'nodes' secdist section is empty");
  }
  
  if(!nodes.IsArray()){
      throw userver::storages::secdist::SecdistError("'nodes' secdist wrong format");
  }
  _nodes = nodes.As<std::vector<NodeDescription>>();
}
}  // namespace cassandra
