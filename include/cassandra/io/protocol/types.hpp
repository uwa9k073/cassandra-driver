#pragma once

#include <vector>
#include <span>

namespace cassandra::io::protocol {

using RawBuffer = std::vector<std::byte>;
using RawBufferView = std::span<std::byte>;

}  // namespace cassandra::io::protocol
