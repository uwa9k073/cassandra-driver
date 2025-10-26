#pragma once
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <memory>

namespace cassandra {
namespace detail {
class SessionImpl;
using SessionImplPtr = std::unique_ptr<SessionImpl>;
}  // namespace detail
class Session {
 public:
  virtual ~Session() = default;

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
  detail::SessionImplPtr pimpl_;
};
}  // namespace cassandra
