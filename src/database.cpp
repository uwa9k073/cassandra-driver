#include <cassandra/database.hpp>

namespace cassandra {
SessionPtr Database::GetSession() const { return _session; }
}  // namespace cassandra
