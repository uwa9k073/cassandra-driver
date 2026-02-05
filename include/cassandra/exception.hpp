#pragma once

#include <stdexcept>
#include <string_view>
namespace cassandra::exceptions {
class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};

class RuntimeError : public Error {
    using Error::Error;
};

class SessionError : public RuntimeError {
    using RuntimeError::RuntimeError;
};

class InvalidConfig : public RuntimeError {
    using RuntimeError::RuntimeError;
};
class ConnectionError : public RuntimeError {
    using RuntimeError::RuntimeError;
};
class PoolError : public RuntimeError {
public:
    PoolError(std::string_view msg, std::string_view keyspace);
    PoolError(std::string_view msg);
};

}  // namespace cassandra::exceptions
