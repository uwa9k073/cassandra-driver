#pragma once

#include <memory>
#include <chrono>

namespace cassandra {
class QueryParameters;
class Query;
class ResultSet;
class Row;

class Session;
using SessionPtr = std::shared_ptr<Session>;

namespace detail {
class Connection;
class ConnectionPool;
class ConnectionPtr;

class ResultWrapper;
using ResultWrapperPtr = std::shared_ptr<const ResultWrapper>;
}  // namespace detail

using TimeoutDuration = std::chrono::milliseconds;

class DefaultCommandControls;
}  // namespace cassandra
