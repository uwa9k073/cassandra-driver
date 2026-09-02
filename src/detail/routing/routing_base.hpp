#pragma once

#include <detail/connection_pool.hpp>
#include <memory>
namespace cassandra::detail::routing {

class PolicyBase {
public:
    PolicyBase() = default;

    virtual std::shared_ptr<ConnectionPool> FindPool() = 0;

    virtual ~PolicyBase() = default;
};
}  // namespace cassandra::detail::routing
