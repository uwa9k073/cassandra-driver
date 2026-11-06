#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/optional_values.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/uuid.hpp>
#include <userver/utils/boost_uuid4.hpp>

// Optional Value Tests (Empty and NonEmpty)
UTEST(OptionalValuesIO, Empty) {
    std::optional<int> opt_int = std::nullopt;
    std::optional<std::string> opt_str = std::nullopt;

    EXPECT_FALSE(opt_int.has_value());
    EXPECT_FALSE(opt_str.has_value());

    cassandra::io::Bytes int_opt_buffer;
    cassandra::io::Bytes str_opt_buffer;

    cassandra::io::WriteBuffer(int_opt_buffer, opt_int);
    cassandra::io::WriteBuffer(str_opt_buffer, opt_str);

    // works ok
    EXPECT_EQ(int_opt_buffer.payload.index(), 0);
    EXPECT_EQ(str_opt_buffer.payload.index(), 0);

    // give me compilation error
    EXPECT_THAT(
        int_opt_buffer.payload,
        testing::VariantWith<cassandra::io::NullTag>(cassandra::io::kNullTag)
    );
    EXPECT_THAT(
        str_opt_buffer.payload,
        testing::VariantWith<cassandra::io::NullTag>(cassandra::io::kNullTag)
    );
}

UTEST(OptionalValuesIO, NonEmpty) {
    std::optional<int> opt_int = 10;
    std::optional<std::string> opt_str = "name";

    EXPECT_TRUE(opt_int.has_value());
    EXPECT_TRUE(opt_str.has_value());

    cassandra::io::Bytes int_opt_buffer;
    cassandra::io::Bytes str_opt_buffer;

    cassandra::io::WriteBuffer(int_opt_buffer, opt_int);
    cassandra::io::WriteBuffer(str_opt_buffer, opt_str);

    // works ok
    EXPECT_EQ(int_opt_buffer.payload.index(), 1);
    EXPECT_EQ(str_opt_buffer.payload.index(), 1);

    cassandra::io::Bytes int_buffer;
    cassandra::io::Bytes str_buffer;
    cassandra::io::WriteBuffer(int_buffer, opt_int.value());
    cassandra::io::WriteBuffer(str_buffer, opt_str.value());

    using Buf = cassandra::io::Bytes::UnderlyingType;

    // give me compilation error
    EXPECT_THAT(
        int_opt_buffer.payload,
        testing::VariantWith<Buf>(std::get<Buf>(int_buffer.payload))
    );
    EXPECT_THAT(
        str_opt_buffer.payload,
        testing::VariantWith<Buf>(std::get<Buf>(str_buffer.payload))
    );
}