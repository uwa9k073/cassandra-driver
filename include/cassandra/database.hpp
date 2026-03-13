#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/database_fwd.hpp>

namespace components {
class Cassandra;
}

namespace cassandra {
class Database {
public:
    SessionPtr GetSessionPtr() const;

private:
    friend class components::Cassandra;
    SessionPtr _session;
};
}  // namespace cassandra
