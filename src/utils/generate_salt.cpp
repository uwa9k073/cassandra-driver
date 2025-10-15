#include "generate_salt.hpp"

#include <userver/utils/rand.hpp>

namespace utils {
std::string GenerateSalt() {
  const int kSaltLength = 32;
  std::string salt;
  for (size_t i = 0; i < kSaltLength; ++i) {
    salt += userver::utils::RandRange(32, 126);
  }
  return salt;
}
}  // namespace utils
