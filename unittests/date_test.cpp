#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/date.hpp>
#include <userver/utest/utest.hpp>

UTEST(DateIO, RoundTrip) {
    using namespace std::chrono_literals;
    // January 1st, 1970 (Unix Epoch)
    const cassandra::io::Date expected_date =
        userver::utils::datetime::DateFromRFC3339String("1970-01-01");

    cassandra::io::Bytes date_bytes;
    cassandra::io::WriteBuffer(date_bytes, expected_date);

    // Assert that the variant index matches your buffer's active type
    EXPECT_EQ(date_bytes.payload.index(), 1);

    EXPECT_EQ(std::get<1>(date_bytes.payload).size(), sizeof(cassandra::io::UInt));

    const auto& actual_date =
        cassandra::io::ReadBuffer<cassandra::io::Date>(date_bytes);

    EXPECT_EQ(actual_date, expected_date);
}
