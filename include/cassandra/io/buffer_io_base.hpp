#pragma once

#include <userver/utils/void_t.hpp>
namespace cassandra::io::detail {
template <typename T, typename Enable = userver::utils::void_t<>>
struct BufferParser;
}  // namespace cassandra::io::detail
