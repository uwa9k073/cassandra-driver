#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <userver/engine/deadline.hpp>

#include "../query.hpp"
#include "../result_set.hpp"

namespace cql::detail {

class PreparedStatementImpl {
 public:
  using StatementId = std::vector<std::uint8_t>;

  PreparedStatementImpl(StatementId id, std::string query);
  virtual ~PreparedStatementImpl() = default;

  /// Execute the prepared statement with given parameters
  virtual ResultSet Execute(const std::vector<Query::Parameter>& parameters,
                            userver::engine::Deadline deadline) = 0;

  /// Get the prepared statement ID
  const StatementId& GetId() const { return id_; }

  /// Get the query string that was prepared
  const std::string& GetQuery() const { return query_; }

  /// Check if this statement is still valid
  virtual bool IsValid() const = 0;

 private:
  StatementId id_;
  std::string query_;
};

}  // namespace cql::detail
