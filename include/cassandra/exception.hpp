#pragma once

#include <fmt/core.h>
#include <cassandra/io/protocol/types.hpp>
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
    FrameError(io::protocol::ErrorCode error_code, std::string_view msg)
        : RuntimeError(fmt::format(
              "Received error message with code: {} (message: {})",
              static_cast<io::Int>(error_code),
              msg
          )),
          _error_code(error_code),
          _error_message(std::string{msg.data(), msg.size()}) {}

    io::protocol::ErrorCode GetErrorCode() const { return _error_code; }
    std::string_view GetErrorMessage() const { return _error_message; }

private:
    io::protocol::ErrorCode _error_code;
    std::string _error_message;
};

class ServerError final : public FrameError {
public:
    explicit ServerError(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kServerError, msg) {}
};

class ProtocolError final : public FrameError {
public:
    explicit ProtocolError(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kProtocolError, msg) {}
};

class AuthenticationError final : public FrameError {
public:
    explicit AuthenticationError(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kAuthenticationError, msg) {}
};

// Availability / execution
class Unavailable final : public FrameError {
public:
    explicit Unavailable(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kUnavailable, msg) {}
};

class Overloaded final : public FrameError {
public:
    explicit Overloaded(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kOverloaded, msg) {}
};

class IsBootstrapping final : public FrameError {
public:
    explicit IsBootstrapping(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kIsBootstrapping, msg) {}
};

class TruncateError final : public FrameError {
public:
    explicit TruncateError(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kTruncateError, msg) {}
};

// Timeouts
class WriteTimeout final : public FrameError {
public:
    explicit WriteTimeout(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kWriteTimeout, msg) {}
};

class ReadTimeout final : public FrameError {
public:
    explicit ReadTimeout(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kReadTimeout, msg) {}
};

// Failures
class ReadFailure final : public FrameError {
public:
    explicit ReadFailure(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kReadFailure, msg) {}
};

class FunctionFailure final : public FrameError {
public:
    explicit FunctionFailure(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kFunctionFailure, msg) {}
};

class WriteFailure final : public FrameError {
public:
    explicit WriteFailure(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kWriteFailure, msg) {}
};

// Query issues
class SyntaxError final : public FrameError {
public:
    explicit SyntaxError(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kSyntaxError, msg) {}
};

class Unauthorized final : public FrameError {
public:
    explicit Unauthorized(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kUnauthorized, msg) {}
};

class Invalid final : public FrameError {
public:
    explicit Invalid(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kInvalid, msg) {}
};

class ConfigError final : public FrameError {
public:
    explicit ConfigError(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kConfigError, msg) {}
};

class AlreadyExists final : public FrameError {
public:
    explicit AlreadyExists(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kAlreadyExists, msg) {}
};

class Unprepared final : public FrameError {
public:
    explicit Unprepared(std::string_view msg)
        : FrameError(io::protocol::ErrorCode::kUnprepared, msg) {}
};

}  // namespace cassandra::exceptions
