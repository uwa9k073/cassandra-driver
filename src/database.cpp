#include <cassandra/database.hpp>
#include <stdexcept>

namespace cassandra {
SessionPtr Database::GetSession() const { return GetSessionForShard(0); }

SessionPtr Database::GetSessionForShard(std::size_t shard) const {
  if (shard >= GetShardCount()) {
    throw std::runtime_error("Node unavailable");
  }
  return _sessions[shard];
}

std::size_t Database::GetShardCount() const { return _sessions.size(); }
}  // namespace cassandra
