#include <cassandra/detail/connection_ptr.hpp>
#include <detail/connection.hpp>
#include <detail/connection_pool.hpp>

namespace cassandra::detail {
ConnectionPtr::ConnectionPtr(std::unique_ptr<Connection>&& conn)
    : _connection_ptr(std::move(conn)) {}

ConnectionPtr::ConnectionPtr(
    Connection* conn, std::shared_ptr<ConnectionPool>&& pool
)
    : _pool_ptr(std::move(pool)), _connection_ptr(conn) {
    UASSERT_MSG(_pool_ptr, "This constructor requires non-empty parent pool");
}

ConnectionPtr::~ConnectionPtr() { Reset(nullptr, nullptr); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept {
    Reset(std::move(other._connection_ptr), std::move(other._pool_ptr));
}

ConnectionPtr& ConnectionPtr::operator=(ConnectionPtr&& other) noexcept {
    Reset(std::move(other._connection_ptr), std::move(other._pool_ptr));
    return *this;
}

ConnectionPtr::operator bool() const noexcept { return _connection_ptr != nullptr; }

Connection* ConnectionPtr::get() const noexcept { return _connection_ptr.get(); }

Connection& ConnectionPtr::operator*() const {
    UASSERT_MSG(_connection_ptr, "Dereferencing null connection pointer");
    return *_connection_ptr;
}

Connection* ConnectionPtr::operator->() const noexcept {
    return _connection_ptr.get();
}

void ConnectionPtr::Reset(
    std::unique_ptr<Connection> conn, std::shared_ptr<ConnectionPool> pool
) {
    Release();
    _connection_ptr = std::move(conn);
    _pool_ptr = std::move(pool);
}

void ConnectionPtr::Release() {
    // We release pooled connection but reset standalone one
    if (_pool_ptr) {
        // _pool_ptr->Release(_connection_ptr.release());
    } else {
        _connection_ptr.reset();
    }
}
}  // namespace cassandra::detail
