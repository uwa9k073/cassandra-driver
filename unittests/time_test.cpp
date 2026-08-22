#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/time.hpp>
#include <userver/utest/utest.hpp>

UTEST(TimeIO, RoundTrip) {
    const cassandra::io::Time expected_time = cassandra::io::Time::FromHHMMInt(1330);

    cassandra::io::Bytes time_bytes;
    cassandra::io::WriteBuffer(time_bytes, expected_time);

    // Assert that the variant index matches your buffer's active type
    EXPECT_EQ(time_bytes.payload.index(), 1);

    EXPECT_EQ(
        std::get<1>(time_bytes.payload).size(), sizeof(cassandra::io::UBigInt)
    );

    const auto& actual_time =
        cassandra::io::ReadBuffer<cassandra::io::Time>(time_bytes);

    EXPECT_EQ(actual_time, expected_time);
}
