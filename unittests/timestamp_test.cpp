#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/timestamp.hpp>
#include <chrono>
#include <optional>
#include <userver/utest/utest.hpp>
#include <userver/utils/datetime_light.hpp>
#include "utils.hpp"

UTEST(TimestampIO, ReadWrite) {
    using cassandra::unittests::utils::ReadWriteBytesTest;
    using namespace cassandra::io;
    using namespace std::chrono;

    std::size_t expected_size = sizeof(BigInt);

    ReadWriteBytesTest<Timestamp>(
        Timestamp{duration_cast<Timestamp::duration>(
            userver::utils::datetime::Now().time_since_epoch()
        )},
        1,
        std::make_optional(expected_size)
    );
}
