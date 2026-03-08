#pragma once

#include <string>
#include <string_view>

namespace cassandra_driver_example {

enum class UserType { kFirstTime, kKnown };

std::string SayHelloTo(std::string_view name, UserType type);

}  // namespace cassandra_driver_example