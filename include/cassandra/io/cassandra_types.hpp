#pragma once

#include <cstddef>
#include <cstdint>
#include <userver/utils/datetime/date.hpp>
#include <userver/utils/ip.hpp>
#include <vector>

namespace cassandra::io {
using BigInt = std::int64_t;
using Int = std::int32_t;
using SmallInt = std::int16_t;
using TinyInt = std::int8_t;

using Boolean = bool;

using Double = double;
using Float = float;

using Inet = userver::utils::ip::InetNetwork;

using Date = userver::utils::datetime::Date;

using Blob = std::vector<std::byte>;
}  // namespace cassandra::io
