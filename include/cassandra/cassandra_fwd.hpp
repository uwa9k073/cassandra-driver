#pragma once

#include <functional>
#include <memory>

namespace cassandra {
class QueryParameters;
class Query;
class ResultSet;
class Row;

class Session;
using SessionPtr = std::shared_ptr<Session>;

namespace detail {
class Connection;
class ConnectionImpl;
class ConnectionPtr;
using ConnectionCallback = std::function<void(Connection*)>;

class ResultWrapper;
using ResultWrapperPtr = std::shared_ptr<const ResultWrapper>;
}  // namespace detail

using TimeoutDuration = std::chrono::milliseconds;

class DefaultCommandControls;
}  // namespace cassandra
