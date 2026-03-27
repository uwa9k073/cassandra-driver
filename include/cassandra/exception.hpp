#pragma once

#include <fmt/core.h>
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
    using RuntimeError::RuntimeError;
    PoolError(std::string_view msg, std::string_view keyspace)
        : RuntimeError(fmt::format("{} (keyspace: {})", msg, keyspace)) {}
    PoolError(std::string_view msg) : RuntimeError({msg.data(), msg.size()}) {}
};

class FrameError : public RuntimeError {
public:
    FrameError(int error_code, std::string_view msg)
        : RuntimeError({msg.data(), msg.size()}),
          _error_code(error_code),
          _error_message(msg) {}

    int GetErrorCode() const { return _error_code; }
    std::string_view GetErrorMessage() const { return _error_message; }

private:
    int _error_code;
    std::string _error_message;
};

}  // namespace cassandra::exceptions
