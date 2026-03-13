#pragma once

#include <fmt/core.h>
#include <array>
#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>

namespace cassandra {
class QueryParameters {
public:
    QueryParameters() = default;

    template <class ParamsHolder>
    explicit QueryParameters(ParamsHolder& ph) : _values(ph.ParamBuffers()) {}
    const io::protocol::RawBuffer* ParamBuffers() { return _values; }

    bool Empty() const { return !_size; }
    std::size_t Size() const { return _size; }

private:
    std::size_t _size = 0;
    const io::protocol::RawBuffer* _values = {};
};

namespace detail {

template <std::size_t ParamsCount>
class StaticQueryParameters {
public:
    StaticQueryParameters() = default;
    StaticQueryParameters(const StaticQueryParameters&) = delete;
    StaticQueryParameters(StaticQueryParameters&&) = delete;
    StaticQueryParameters& operator=(const StaticQueryParameters&) = delete;
    StaticQueryParameters& operator=(StaticQueryParameters&&) = delete;

    const io::protocol::RawBuffer* ParamBuffers() { return _args.data(); }

    // i need also bind paramter names
    template <typename T>
    void Write(std::size_t index, const T& arg) {
        // add some checks for parameter type mapping
        io::BufferWriter writer(_args[index]);
        writer.Write(arg);
    }

    // used with Session::Execute(ConsistencyLevel level, LongString query, Args...
    // args) this binds only the values, not the names
    template <typename... Args>
    void Write(const Args&... args) {
        if constexpr (sizeof...(Args) != ParamsCount) {
            throw exceptions::Error(fmt::format(
                "Invalid number of arguments: expected {}, got {}",
                ParamsCount,
                sizeof...(Args)
            ));
        }
        std::size_t index = 0;
        (Write(index++, args), ...);
    }

private:
    std::array<io::protocol::RawBuffer, ParamsCount> _args;
};

template <>
class StaticQueryParameters<0> {
public:
    static std::size_t Size() { return 0; }
    static const io::protocol::RawBuffer* ParamBuffers() { return nullptr; }

    static void Write() {}
};
}  // namespace detail
}  // namespace cassandra
