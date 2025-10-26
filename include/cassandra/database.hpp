#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/database_fwd.hpp>
#include <cstddef>
#include <vector>

namespace components {
class Cassandra;
}

namespace cassandra {
class Database {
 public:
  SessionPtr GetSession() const;
  SessionPtr GetSessionForShard(std::size_t shard) const;

  std::size_t GetShardCount() const;

 private:
  friend class components::Cassandra;
  std::vector<SessionPtr> _sessions;
};
}  // namespace cassandra
