#include "result_set.hpp"
#include "detail/result_set_impl.hpp"

namespace cql {

ResultSet::ResultSet(std::unique_ptr<detail::ResultSetImpl>&& impl)
    : impl_(std::move(impl)) {}

ResultSet::~ResultSet() = default;

ResultSet::ResultSet(ResultSet&&) noexcept = default;

ResultSet& ResultSet::operator=(ResultSet&&) noexcept = default;

std::optional<ResultSet::Row> ResultSet::FetchRow() {
  return impl_->FetchRow();
}

std::size_t ResultSet::RemainingRows() const { return impl_->RemainingRows(); }

const std::string& ResultSet::GetColumnName(std::size_t idx) const {
  return impl_->GetColumnName(idx);
}

std::size_t ResultSet::GetColumnCount() const {
  return impl_->GetColumnCount();
}

}  // namespace cql
