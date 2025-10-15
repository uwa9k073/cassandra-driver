#pragma once

#include <string>
#include <vector>
#include <optional>

#include <userver/utils/str_icase.hpp>
#include <userver/engine/io/socket.hpp>

namespace cql {

/// Data source name for CQL connection
class Dsn {
 public:
  /// Parse DSN string in format:
  /// cql://[username:password@]host[:port][/keyspace][?param1=value1&param2=value2]
  explicit Dsn(const ::std::string& dsn_string);

  /// Construct DSN from components
  Dsn(::std::string host, uint16_t port,
      ::std::optional<::std::string> keyspace = ::std::nullopt,
      ::std::optional<::std::string> username = ::std::nullopt,
      ::std::optional<::std::string> password = ::std::nullopt);

  /// Get connection host
  const ::std::string& GetHost() const { return host_; }

  /// Get connection port
  uint16_t GetPort() const { return port_; }

  /// Get keyspace name if set
  const ::std::optional<::std::string>& GetKeyspace() const {
    return keyspace_;
  }

  /// Get username if set
  const ::std::optional<::std::string>& GetUsername() const {
    return username_;
  }

  /// Get password if set
  const ::std::optional<::std::string>& GetPassword() const {
    return password_;
  }

  /// Get parameter value by name
  ::std::optional<::std::string> GetParameter(const ::std::string& name) const;

  /// Convert DSN to string (hiding password)
  ::std::string ToString() const;

  bool operator==(const Dsn& other) const {
    return userver::utils::StrIcaseEqual()(host_, other.host_) &&
           port_ == other.port_ && keyspace_ == other.keyspace_ &&
           username_ == other.username_ && password_ == other.password_ &&
           parameters_ == other.parameters_;
  }

  bool operator!=(const Dsn& other) const { return !(*this == other); }

 private:
  void Parse(const ::std::string& dsn_string);
  void ParseParameters(const ::std::string& params);

  ::std::string host_;
  uint16_t port_{9042};  // Default CQL port
  ::std::optional<::std::string> keyspace_;
  ::std::optional<::std::string> username_;
  ::std::optional<::std::string> password_;
  ::std::vector<::std::pair<::std::string, ::std::string>> parameters_;
};
}  // namespace cql
