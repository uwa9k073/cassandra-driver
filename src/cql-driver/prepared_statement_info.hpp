#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cql {

/// @brief Class containing metadata about a prepared statement
class PreparedStatementInfo {
 public:
  /// Column metadata
  struct Column {
    std::string keyspace;  ///< Keyspace name
    std::string table;     ///< Table name
    std::string name;      ///< Column name
    int32_t type;          ///< CQL type code
  };

  /// Create PreparedStatementInfo with prepared statement ID and metadata
  PreparedStatementInfo(std::vector<std::uint8_t> id,
                        std::vector<Column> result_metadata,
                        std::vector<Column> parameter_metadata)
      : id_(std::move(id)),
        result_metadata_(std::move(result_metadata)),
        parameter_metadata_(std::move(parameter_metadata)) {}

  /// Get the prepared statement ID
  const std::vector<std::uint8_t>& GetId() const { return id_; }

  /// Get result column metadata
  const std::vector<Column>& GetResultMetadata() const {
    return result_metadata_;
  }

  /// Get parameter metadata
  const std::vector<Column>& GetParameterMetadata() const {
    return parameter_metadata_;
  }

 private:
  std::vector<std::uint8_t> id_;
  std::vector<Column> result_metadata_;
  std::vector<Column> parameter_metadata_;
};
}  // namespace cql
