#pragma once

#include <cassandra/io/protocol/types.hpp>
#include <cassandra/row.hpp>

namespace cassandra {
class ResultSet {
public:
private:
    io::protocol::RawBuffer buffer;
};
}  // namespace cassandra
