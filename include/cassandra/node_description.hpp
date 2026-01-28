#pragma once

#include <cstdint>
#include <optional>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
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

inline PasswordAuthentificator Parse(
    const userver::formats::json::Value& value,
    userver::formats::parse::To<PasswordAuthentificator>) {
  return {.username = value["username"].As<std::string>(),
          .password = value["password"].As<Password>()};
}
inline NodeDescription Parse(const userver::formats::json::Value& value,
                      userver::formats::parse::To<NodeDescription>) {
  NodeDescription node_description;
  node_description.use_ssl = value["use-ssl"].As<bool>(false);
  node_description.use_compression = value["use-compression"].As<bool>();
  node_description.allow_all = value["allow-all"].As<bool>(true);
  auto creds = value["auth"].As<std::optional<PasswordAuthentificator>>();

  if (!node_description.allow_all && creds.has_value()) {
    node_description.password_authetificator =
        value["auth"].As<PasswordAuthentificator>();
  } else if ((node_description.allow_all && creds) || (!node_description.allow_all && !creds)){
      throw userver::storages::secdist::SecdistError("Cassandra node auth configuration error");
  }
  node_description.contact_point =
      value["contact-point"].As<std::string>();
  node_description.contact_point =
      value["port"].As<std::int64_t>(9042);
  return node_description;
}
}  // namespace cassandra
