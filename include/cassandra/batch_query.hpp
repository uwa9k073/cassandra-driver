#pragma once

#include <cassandra/detail/query_parameters.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/query.hpp>
#include <string_view>
#include <userver/storages/query.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/zstring_view.hpp>
#include <utility>

namespace cassandra {

class BatchQuery {
public:
    template <typename... Args>
    constexpr BatchQuery(Query query, Args&&... args) : _query{std::move(query)} {
        detail::StaticQueryParameters<sizeof...(Args)> params;
        params.Write(args...);
        _params = std::move(params);
    }

    template <typename... Args>
    constexpr BatchQuery(std::string_view query, Args&&... args)
        : _query{std::move(query)} {
        detail::StaticQueryParameters<sizeof...(Args)> params;
        params.Write(args...);
        _params = std::move(params);
    }

    template <typename... Args>
    constexpr BatchQuery(userver::utils::StringLiteral query, Args&&... args)
        : _query{std::move(query)} {
        detail::StaticQueryParameters<sizeof...(Args)> params;
        params.Write(args...);
        _params = std::move(params);
    }

private:
    Query _query;
    QueryParameters _params;
};

class BatchQueryStore {
public:
    template <typename... Args>
    constexpr BatchQueryStore& AddQuery(Query query, Args&&... args) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args>
    constexpr BatchQueryStore& AddQuery(
        userver::utils::StringLiteral query, Args&&... args
    ) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args>
    constexpr BatchQueryStore& AddQuery(std::string_view query, Args&&... args) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

private:
    std::vector<BatchQuery> _queries;
};
}  // namespace cassandra
