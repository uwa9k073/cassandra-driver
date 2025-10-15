#include <userver/crypto/algorithm.hpp>
#include <userver/utest/utest.hpp>
#include <utils/generate_salt.hpp>

using namespace userver;

UTEST(GenerateSalt, GenerateDiffererent) {
  std::string salt1 = ::utils::GenerateSalt();
  std::string salt2 = ::utils::GenerateSalt();

  auto exp = crypto::algorithm::AreStringsEqualConstTime(salt1, salt2);

  ASSERT_FALSE(exp);
}
