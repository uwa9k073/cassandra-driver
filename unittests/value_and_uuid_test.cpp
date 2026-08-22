#include <list>
#include <map>
#include <unordered_map>
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

// Value and UUID Tests (Formatting and UUID)
UTEST(ValueIO, Formatting) {
    std::optional<int> opt_int = std::nullopt;
    std::optional<std::string> opt_str = std::nullopt;

    EXPECT_FALSE(opt_int.has_value());
    EXPECT_FALSE(opt_str.has_value());

    cassandra::io::Value int_opt_buffer;
    cassandra::io::Value str_opt_buffer;

    cassandra::io::WriteBuffer(int_opt_buffer, opt_int);
    cassandra::io::WriteBuffer(str_opt_buffer, opt_str);

    // works ok
    EXPECT_EQ(int_opt_buffer.payload.index(), 1);
    EXPECT_EQ(str_opt_buffer.payload.index(), 1);

    // give me compilation error
    EXPECT_THAT(
        int_opt_buffer.payload,
        testing::VariantWith<cassandra::io::Bytes>(
            cassandra::io::Bytes{.payload = cassandra::io::kNullTag}
        )
    );
    EXPECT_THAT(
        str_opt_buffer.payload,
        testing::VariantWith<cassandra::io::Bytes>(
            cassandra::io::Bytes{.payload = cassandra::io::kNullTag}
        )
    );
}

UTEST(Uuid, FormattingBytes) {
    const auto& uuid = userver::utils::generators::GenerateBoostUuid();

    cassandra::io::Bytes uuid_bytes;
    cassandra::io::WriteBuffer(uuid_bytes, uuid);

    EXPECT_EQ(uuid_bytes.payload.index(), 1);
    EXPECT_EQ(std::get<1>(uuid_bytes.payload).size(), 16);
}

UTEST(Uuid, FormattingValue) {
    const auto& uuid = userver::utils::generators::GenerateBoostUuid();

    cassandra::io::Value uuid_value;
    cassandra::io::WriteBuffer(uuid_value, uuid);

    EXPECT_EQ(uuid_value.payload.index(), 1);
    EXPECT_EQ(std::get<1>(std::get<1>(uuid_value.payload).payload).size(), 16);
}