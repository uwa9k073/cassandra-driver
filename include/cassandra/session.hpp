#pragma once
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <memory>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include "cassandra/node_description.hpp"

namespace cassandra {
namespace detail {
class SessionImpl;
using SessionImplPtr = std::unique_ptr<SessionImpl>;
}  // namespace detail
class Session {
 public:
  Session(std::vector<NodeDescription> node_description,
          userver::clients::dns::Resolver* resolver,
          userver::engine::TaskProcessor& task_processor,
          userver::utils::statistics::MetricsStoragePtr metrics_storage);

  /// Execute a query
  template <typename... T>
  ResultSet Execute(const Query& query, const T&... args);

  /// Execute a query with specific command control
  template <typename... T>
  ResultSet Execute(CommandControl command_ctl, const Query& query,
                    const T&... args);

  /// Execute a prepared statement
  ResultSet ExecutePrepared(const std::string& query_id,
                            const QueryParameters& params);

  /// Prepare a statement
  std::string Prepare(const Query& query);

  /// Check if connected
  bool IsConnected() const;

 private:
  detail::SessionImplPtr _pimpl;
};
}  // namespace cassandra
