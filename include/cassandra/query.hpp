#pragma once

#include <userver/storages/query.hpp>

namespace cassandra {
class Query {
 public:
  Query() = default;
  ~Query() = default;

  Query(const Query& other) = default;
  Query(Query&& other) = default;
  Query& operator=(const Query& other) = default;
  Query& operator=(Query&& other) = default;

  constexpr Query(userver::utils::StringLiteral statement)
      : data_{StaticStrings{statement}} {}

  Query(const char* statement) : Query(std::string{statement}) {}
  Query(std::string statement) : data_{DynamicStrings{std::move(statement)}} {}

 private:
  struct DynamicStrings {
    std::string statement;
  };
  struct StaticStrings {
    userver::utils::StringLiteral statement;
  };

  std::variant<StaticStrings, DynamicStrings> data_ =
      StaticStrings{userver::utils::StringLiteral{""}};
};
}  // namespace cassandra
