#pragma once

#include <memory>

namespace cassandra {
class Database;
using DatabasePtr = std::shared_ptr<Database>;
}  // namespace cassandra
