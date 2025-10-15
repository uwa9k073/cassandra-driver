#include "query.hpp"

namespace cql {

Query::Query(std::string query, Consistency consistency)
    : query_(std::move(query)), consistency_(consistency) {}

Query& Query::AddParameter(Parameter value) {
  parameters_.push_back(std::move(value));
  return *this;
}

}  // namespace cql
