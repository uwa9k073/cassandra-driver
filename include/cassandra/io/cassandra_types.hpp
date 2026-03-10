#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <userver/utils/datetime/date.hpp>
#include <userver/utils/ip.hpp>
#include <userver/utils/strong_typedef.hpp>
#include <vector>

// here we declare scalar types described in the Cassandra Native Protocol V4
// here we dont declare such types as string list, string map and etc described in the protocol notations
namespace cassandra::io {
using BigInt = std::int64_t;
using Int = std::int32_t;
using SmallInt = std::int16_t;
using TinyInt = std::int8_t;

using Short = std::uint16_t;

using Boolean = bool;

using Double = double;
using Float = float;

using Inet = userver::utils::ip::InetNetwork;

using Date = userver::utils::datetime::Date;

using Blob = std::vector<std::byte>;

using String = userver::utils::StrongTypedef<Short, std::string>;
using LongString = userver::utils::StrongTypedef<Int, std::string>;

using StringList = std::vector<String>;

using StringMap = std::unordered_map<String, String>;
using StringMultiMap = std::unordered_map<String, StringList>;
}  // namespace cassandra::io
