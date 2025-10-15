#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <variant>

namespace cql {

class Query {
 public:
  /// Parameter types supported by CQL
  using Parameter =
      std::variant<std::int32_t, std::int64_t, float, double, bool, std::string,
                   std::vector<std::uint8_t>  // For blob data
                   >;

  /// Query consistency level
  enum class Consistency {
    ANY,
    ONE,
    TWO,
    THREE,
    QUORUM,
    ALL,
    LOCAL_QUORUM,
    EACH_QUORUM,
    LOCAL_ONE
  };

  /// Create a query with specified consistency level
  explicit Query(std::string query,
                 Consistency consistency = Consistency::LOCAL_ONE);

  /// Add a parameter to the query
  Query& AddParameter(Parameter value);

  /// Get the query string
  const std::string& GetQuery() const { return query_; }

  /// Get the parameters
  const std::vector<Parameter>& GetParameters() const { return parameters_; }

  /// Get the consistency level
  Consistency GetConsistency() const { return consistency_; }

 private:
  std::string query_;
  std::vector<Parameter> parameters_;
  Consistency consistency_;
};

}  // namespace cql
