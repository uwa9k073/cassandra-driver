#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cql::detail {

/// @brief Implementation of ResultSet functionality
class ResultSetImpl {
 public:
  using Row = std::vector<std::vector<std::uint8_t>>;

  virtual ~ResultSetImpl() = default;

  /// Get next row from the result set
  virtual std::optional<Row> FetchRow() = 0;

  /// Get the number of remaining rows
  virtual std::size_t RemainingRows() const = 0;

  /// Get column name by index
  virtual const std::string& GetColumnName(std::size_t idx) const = 0;

  /// Get total number of columns
  virtual std::size_t GetColumnCount() const = 0;
};

}  // namespace cql::detail
