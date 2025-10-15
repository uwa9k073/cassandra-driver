#include "prepared_statement_impl.hpp"

namespace cql::detail {

PreparedStatementImpl::PreparedStatementImpl(StatementId id, std::string query)
    : id_(std::move(id)), query_(std::move(query)) {}

}  // namespace cql::detail
