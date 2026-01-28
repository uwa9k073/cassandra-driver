#pragma once

#include <userver/formats/json/value.hpp>
#include <vector>
#include <cassandra/node_description.hpp>
namespace cassandra {
struct CassandraSecdist {
  explicit CassandraSecdist(const userver::formats::json::Value& doc);

 private:
  std::vector<NodeDescription> _nodes;
};
}  // namespace cassandra
