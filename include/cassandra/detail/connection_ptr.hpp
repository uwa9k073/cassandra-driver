#pragma once

#include <memory>
#include "cassandra/cassandra_fwd.hpp"
namespace cassandra::detail {
class ConnectionPtr {
public:
    explicit ConnectionPtr(std::unique_ptr<Connection>&& conn);
    ConnectionPtr(Connection* conn, std::shared_ptr<ConnectionPool>&& pool);
    ~ConnectionPtr();

    ConnectionPtr(ConnectionPtr&&) noexcept;
    ConnectionPtr& operator=(ConnectionPtr&&) noexcept;

    explicit operator bool() const noexcept;
    Connection* get() const noexcept;

    Connection& operator*() const;
    Connection* operator->() const noexcept;

private:
    void Reset(
        std::unique_ptr<Connection> conn, std::shared_ptr<ConnectionPool> pool
    );
    void Release();
    std::shared_ptr<ConnectionPool> _pool_ptr;
    std::unique_ptr<Connection> _connection_ptr;
};
}  // namespace cassandra::detail
