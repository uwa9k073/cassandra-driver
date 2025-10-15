#include "dsn.hpp"

#include <regex>
#include <stdexcept>
#include <cstdio>
#include <userver/utils/str_icase.hpp>

namespace cql {

namespace {

::std::string UrlDecode(const ::std::string& str) {
  ::std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '%' && i + 2 < str.size()) {
      int value = 0;
      if (::sscanf(str.c_str() + i + 1, "%2x", &value) == 1) {
        result += static_cast<char>(value);
        i += 2;
      } else {
        result += str[i];
      }
    } else if (str[i] == '+') {
      result += ' ';
    } else {
      result += str[i];
    }
  }

  return result;
}

}  // namespace

Dsn::Dsn(const ::std::string& dsn_string) { Parse(dsn_string); }

Dsn::Dsn(::std::string host, uint16_t port,
         ::std::optional<::std::string> keyspace,
         ::std::optional<::std::string> username,
         ::std::optional<::std::string> password)
    : host_(::std::move(host)),
      port_(port),
      keyspace_(::std::move(keyspace)),
      username_(::std::move(username)),
      password_(::std::move(password)) {
  if (host_.empty()) {
    throw ::std::invalid_argument("Host cannot be empty");
  }
}

void Dsn::Parse(const ::std::string& dsn_string) {
  static const ::std::regex dsn_regex(
      R"(^cql://(?:([^:@]+)(?::([^@]+))?@)?([^:/]+)(?::(\d+))?(?:/([^?]+))?(?:\?(.*))?$)");

  ::std::smatch match;
  if (!::std::regex_match(dsn_string, match, dsn_regex)) {
    throw ::std::invalid_argument("Invalid DSN format");
  }

  if (match[1].matched) {
    username_ = UrlDecode(match[1].str());
  }
  if (match[2].matched) {
    password_ = UrlDecode(match[2].str());
  }

  host_ = UrlDecode(match[3].str());
  if (host_.empty()) {
    throw ::std::invalid_argument("Host cannot be empty");
  }

  if (match[4].matched) {
    try {
      port_ = static_cast<uint16_t>(::std::stoi(match[4].str()));
    } catch (const ::std::exception& e) {
      throw ::std::invalid_argument("Invalid port number");
    }
  }

  if (match[5].matched) {
    keyspace_ = UrlDecode(match[5].str());
  }

  if (match[6].matched) {
    ParseParameters(match[6].str());
  }
}

void Dsn::ParseParameters(const ::std::string& params) {
  ::std::string::size_type start = 0;
  while (start < params.size()) {
    auto eq_pos = params.find('=', start);
    if (eq_pos == ::std::string::npos) {
      break;
    }

    auto amp_pos = params.find('&', eq_pos);
    if (amp_pos == ::std::string::npos) {
      amp_pos = params.size();
    }

    ::std::string key = UrlDecode(params.substr(start, eq_pos - start));
    ::std::string value =
        UrlDecode(params.substr(eq_pos + 1, amp_pos - eq_pos - 1));
    parameters_.emplace_back(::std::move(key), ::std::move(value));

    start = amp_pos + 1;
  }
}

::std::optional<::std::string> Dsn::GetParameter(
    const ::std::string& name) const {
  for (const auto& [key, value] : parameters_) {
    if (userver::utils::StrIcaseEqual()(key, name)) {
      return value;
    }
  }
  return ::std::nullopt;
}

::std::string Dsn::ToString() const {
  ::std::string result = "cql://";

  if (username_) {
    result += *username_;
    if (password_) {
      result += ":***";  // Hide password
    }
    result += '@';
  }

  result += host_;

  if (port_ != 9042) {
    result += ':' + ::std::to_string(port_);
  }

  if (keyspace_) {
    result += '/' + *keyspace_;
  }

  if (!parameters_.empty()) {
    result += '?';
    bool first = true;
    for (const auto& [key, value] : parameters_) {
      if (!first) {
        result += '&';
      }
      result += key + '=' + value;
      first = false;
    }
  }

  return result;
}
}  // namespace cql
