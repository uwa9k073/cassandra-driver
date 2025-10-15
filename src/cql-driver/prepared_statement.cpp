#include "prepared_statement.hpp"
#include "detail/prepared_statement_impl.hpp"

namespace cql {

PreparedStatement::PreparedStatement(
    std::unique_ptr<detail::PreparedStatementImpl>&& impl)
    : impl_(std::move(impl)) {}

PreparedStatement::~PreparedStatement() = default;

PreparedStatement::PreparedStatement(PreparedStatement&&) noexcept = default;

PreparedStatement& PreparedStatement::operator=(PreparedStatement&&) noexcept =
    default;

ResultSet PreparedStatement::Execute(
    const std::vector<Query::Parameter>& parameters,
    userver::engine::Deadline deadline) {
  return impl_->Execute(parameters, deadline);
}

const PreparedStatement::StatementId& PreparedStatement::GetId() const {
  return impl_->GetId();
}

const std::string& PreparedStatement::GetQuery() const {
  return impl_->GetQuery();
}

bool PreparedStatement::IsValid() const { return impl_ && impl_->IsValid(); }

}  // namespace cql
