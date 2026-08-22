#pragma once

#include <fmt/core.h>
#include <array>
#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/floating_point_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/string_types.hpp>
#include <cstddef>
#include <vector>

namespace cassandra {
class QueryParameters {
public:
    QueryParameters() = default;

    template <class ParamsHolder>
    explicit QueryParameters(ParamsHolder& ph)
        : _size(ph.Size()), _values(ph.ParamBuffers()) {}
    const io::Value* ParamBuffers() const { return _values; }

    bool Empty() const { return !_size; }
    std::size_t Size() const { return _size; }

private:
    std::size_t _size = 0;
    const io::Value* _values = {};
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

    const io::Value* ParamBuffers() { return _args.data(); }
    std::size_t Size() const { return ParamsCount; }

    // i need also bind paramter names
    template <typename T>
    void Write(std::size_t index, const T& arg) {
        // add some checks for parameter type mapping
        io::BufferWriter writer(_args[index]);
        writer.Write(arg);
    }

    // used with Session::Execute(ConsistencyLevel level, LongString query,
    // Args... args) this binds only the values, not the names
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
    std::array<io::Value, ParamsCount> _args;
};

template <>
class StaticQueryParameters<0> {
public:
    static std::size_t Size() { return 0; }
    static const io::Value* ParamBuffers() { return nullptr; }

    static void Write() {}
};

class DynamicQueryParameters {
public:
    DynamicQueryParameters() = default;
    DynamicQueryParameters(const DynamicQueryParameters&) = delete;
    DynamicQueryParameters(DynamicQueryParameters&&) = delete;
    DynamicQueryParameters& operator=(const DynamicQueryParameters&) = delete;
    DynamicQueryParameters& operator=(DynamicQueryParameters&&) = delete;

    const io::Value* ParamBuffers() { return _args->data(); }
    std::size_t Size() const { return _args->size(); }

    std::shared_ptr<std::vector<io::Value>> ParamHolder() { return _args; }
    template <typename T>
    void Write(std::size_t index, const T& arg) {
        // add some checks for parameter type mapping
        io::BufferWriter writer(_args->at(index));
        writer.Write(arg);
    }
    template <typename... Args>
    void Write(const Args&... args) {
        _args->resize(sizeof...(args));
        std::size_t index = 0;
        (Write(index++, args), ...);
    }

private:
    std::shared_ptr<std::vector<io::Value>> _args =
        std::make_shared<std::vector<io::Value>>();
};
}  // namespace detail
}  // namespace cassandra
