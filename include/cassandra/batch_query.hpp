#pragma once

#include <cassandra/detail/query_parameters.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/options.hpp>
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
    BatchQuery(const Query& query, Args&&... args) : _query{query} {
        detail::DynamicQueryParameters params;
        params.Write(args...);
        _params = QueryParameters{params};
        _params_holder = params.ParamHolder();
    }

    template <typename... Args>
    BatchQuery(std::string_view query, Args&&... args) : _query{std::move(query)} {
        detail::DynamicQueryParameters params;
        params.Write(args...);
        _params = QueryParameters{params};
        _params_holder = params.ParamHolder();
    }

    template <typename... Args>
    BatchQuery(userver::utils::StringLiteral query, Args&&... args)
        : _query{std::move(query)} {
        detail::DynamicQueryParameters params;
        params.Write(args...);
        _params = QueryParameters{params};
        _params_holder = params.ParamHolder();
    }

    const Query& GetQuery() const { return _query; }
    const QueryParameters& GetParams() const { return _params; }

private:
    Query _query;
    QueryParameters _params;

    std::shared_ptr<std::vector<io::Bytes>> _params_holder;
};

struct BatchStatement {
    enum class Kind : io::Byte { kString = 0, kId = 1 };
    Kind kind;
    std::variant<io::LongString, io::ShortBytes> query;
    QueryParameters params;

    BatchStatement(const io::LongString& query, const QueryParameters& params)
        : kind(Kind::kString), query(query), params(params) {}
    BatchStatement(const io::ShortBytes& id, const QueryParameters& params)
        : kind(Kind::kId), query(id), params(params) {}
};

class BatchQueryStore {
public:
    explicit BatchQueryStore(Consistency level) : _consistency{level} {}

    template <typename... Args>
    [[maybe_unused]] BatchQueryStore& AddQuery(const Query& query, Args&&... args) {
        _queries.emplace_back(query, std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args>
    [[maybe_unused]] BatchQueryStore& AddQuery(
        userver::utils::StringLiteral query, Args&&... args
    ) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args>
    [[maybe_unused]] BatchQueryStore& AddQuery(
        std::string_view query, Args&&... args
    ) {
        _queries.emplace_back(std::move(query), std::forward<Args>(args)...);
        return *this;
    }

    Consistency ConsistencyLevel() const { return _consistency; }
    std::span<const BatchQuery> Queries() const { return _queries; }

private:
    Consistency _consistency;
    std::vector<BatchQuery> _queries;
};
}  // namespace cassandra
