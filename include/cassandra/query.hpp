#pragma once

#include <cassandra/io/cassandra_types.hpp>
#include <userver/storages/query.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/zstring_view.hpp>
#include <utility>

namespace cassandra {
class Query {
public:
    Query() = default;
    ~Query() = default;

    Query(const Query& other) = default;
    Query(Query&& other) = default;
    Query& operator=(const Query& other) = default;
    Query& operator=(Query&& other) = default;

    constexpr Query(std::string_view statement)
        : data_{io::LongString{statement.data(), statement.size()}} {}

    constexpr Query(userver::utils::StringLiteral statement)
        : data_{io::LongString{statement.data(), statement.size()}} {}

    Query(const char* statement) : Query(std::string{statement}) {}
    Query(std::string statement) : data_{io::LongString{std::move(statement)}} {}
    Query(io::LongString&& statement) : data_{std::move(statement)} {}
    Query(const io::LongString& statement) : data_{statement} {}

    io::LongString GetStatement() const { return data_; }

private:
    io::LongString data_;
};
}  // namespace cassandra
