#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cql {

namespace detail {
class ResultSetImpl;
}

/// @brief Class representing a result set from a CQL query
class ResultSet {
 public:
  /// Row data type
  using Row = std::vector<std::vector<uint8_t>>;

  /// Create a result set from implementation
  explicit ResultSet(std::unique_ptr<detail::ResultSetImpl>&& impl);
  ~ResultSet();

  ResultSet(const ResultSet&) = delete;
  ResultSet& operator=(const ResultSet&) = delete;
  ResultSet(ResultSet&&) noexcept;
  ResultSet& operator=(ResultSet&&) noexcept;

  /// Get next row from the result set
  std::optional<Row> FetchRow();

  /// Get the number of remaining rows
  std::size_t RemainingRows() const;

  /// Get column name by index
  const std::string& GetColumnName(std::size_t idx) const;

  /// Get total number of columns
  std::size_t GetColumnCount() const;

 private:
  std::unique_ptr<detail::ResultSetImpl> impl_;
};

}  // namespace cql
