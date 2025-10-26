#pragma once

#include <cstdint>
#include <optional>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/strong_typedef.hpp>

namespace cassandra {
using Password = userver::utils::NonLoggable<class PasswordTag, std::string>;
struct PasswordAuthentificator {
  std::string username;
  Password password;
};
struct NodeDescription {
  bool use_ssl;
  bool use_compression;
  bool allow_all;
  std::optional<PasswordAuthentificator> password_authetificator = std::nullopt;
  std::string contact_point;
  std::uint64_t port;
};
template <class Value>
NodeDescription Parse(const Value& value,
                      userver::formats::parse::To<NodeDescription>) {
  NodeDescription node_description;
  node_description.use_ssl = value["use-ssl"].template As<bool>(false);
  node_description.use_compression =
      value["use-compression"].template As<bool>();
  node_description.allow_all = value["allow-all"].template As<bool>(true);
  if (!node_description.allow_all) {
    node_description.password_authetificator = PasswordAuthentificator{
        .username = value["auth"]["username"].template As<std::string>(),
        .password = value["auht"]["password"].template As<Password>()};
  }
  node_description.contact_point =
      value["contact-point"].template As<std::string>();
  node_description.contact_point =
      value["port"].template As<std::int64_t>(9042);
  return node_description;
}
}  // namespace cassandra
