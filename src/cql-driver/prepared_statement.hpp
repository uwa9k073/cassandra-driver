#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <userver/engine/deadline.hpp>

#include "query.hpp"
#include "result_set.hpp"

namespace cql {

namespace detail {
class PreparedStatementImpl;
}

/// @brief Class representing a prepared statement in CQL
class PreparedStatement {
 public:
  /// The unique ID for this prepared statement
  using StatementId = std::vector<std::uint8_t>;

  /// Create a prepared statement from implementation
  explicit PreparedStatement(
      std::unique_ptr<detail::PreparedStatementImpl>&& impl);
  ~PreparedStatement();

  PreparedStatement(const PreparedStatement&) = delete;
  PreparedStatement& operator=(const PreparedStatement&) = delete;
  PreparedStatement(PreparedStatement&&) noexcept;
  PreparedStatement& operator=(PreparedStatement&&) noexcept;

  /// Execute the prepared statement with given parameters
  ResultSet Execute(const std::vector<Query::Parameter>& parameters,
                    userver::engine::Deadline deadline);

  /// Get the prepared statement ID
  const StatementId& GetId() const;

  /// Get the query string that was prepared
  const std::string& GetQuery() const;

  /// Check if this statement is still valid
  bool IsValid() const;

 private:
  std::unique_ptr<detail::PreparedStatementImpl> impl_;
};
}  // namespace cql
