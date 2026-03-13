#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>

namespace cassandra::detail {
class QueryParameters {
public:
    QueryParameters() = default;

    template <class ParamsHolder>
    explicit QueryParameters(ParamsHolder& ph)
        : _values(ph.ParamBuffers()), _names(ph.ParamNames()) {}

    bool Empty() const { return !_size; }
    std::size_t Size() const { return _size; }

private:
    std::size_t _size = 0;
    const std::byte* const* _values = nullptr;
    const char* const* _names = nullptr;
};

template <std::size_t ParamsCount>
class StaticQueryParameters {
public:
    StaticQueryParameters() = default;
    StaticQueryParameters(const StaticQueryParameters&) = delete;
    StaticQueryParameters(StaticQueryParameters&&) = delete;
    StaticQueryParameters& operator=(const StaticQueryParameters&) = delete;
    StaticQueryParameters& operator=(StaticQueryParameters&&) = delete;
    
    template <typename T>
    void Write(std::size_t index,  const T& arg) {
        
    }
    
    template<typename... Args>
    void Write(const Args&... args){
        std::size_t index = 0;
        (Write(index++, args),...);
    }

private:
};
}  // namespace cassandra::detail
