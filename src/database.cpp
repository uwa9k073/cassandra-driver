#include <cassandra/database.hpp>

namespace cassandra {
SessionPtr Database::GetSessionPtr() const { return _session; }
}  // namespace cassandra
