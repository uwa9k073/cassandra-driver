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

UTEST(ScalarTypesIO, Integral) {
    cassandra::io::protocol::Buffer buffer;

    int value = 42;
    cassandra::io::WriteBuffer(buffer.data, value);
    EXPECT_EQ(4, buffer.data.size());
    auto actual_value = cassandra::io::ReadBuffer<int>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_value, value);
}

UTEST(ScalarTypesIO, Float) {
    cassandra::io::protocol::Buffer buffer;

    float value = 3.14f;
    cassandra::io::WriteBuffer(buffer.data, value);
    EXPECT_EQ(4, buffer.data.size());
    auto actual_value = cassandra::io::ReadBuffer<float>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_value, value);
}

UTEST(ScalarTypesIO, Double) {
    cassandra::io::protocol::Buffer buffer;

    double value = 3.14;
    cassandra::io::WriteBuffer(buffer.data, value);
    EXPECT_EQ(8, buffer.data.size());
    auto actual_value =
        cassandra::io::ReadBuffer<double>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_value, value);
}

UTEST(ListTypesIO, IntegralVector) {
    cassandra::io::protocol::Buffer buffer;

    std::vector<int> expected_values = {1, 2, 3};
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(16, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<std::vector<int>>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

UTEST(ListTypesIO, StringVector) {
    cassandra::io::protocol::Buffer buffer;

    cassandra::io::StringList expected_values = {
        cassandra::io::String("ab"),
        cassandra::io::String("de"),
        cassandra::io::String("gh")
    };
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(14, buffer.data.size());
    auto actual_values = cassandra::io::ReadBuffer<cassandra::io::StringList>(
        buffer.data, buffer.offset
    );
    EXPECT_EQ(actual_values, expected_values);
}

UTEST(ListTypesIO, IntegralList) {
    cassandra::io::protocol::Buffer buffer;

    std::list<int> expected_values = {1, 2, 3};
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(16, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<std::list<int>>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

UTEST(MapTypesIO, IntegralUnorderedMap) {
    cassandra::io::protocol::Buffer buffer;

    using Map = std::unordered_map<int, int>;
    Map expected_values = {{1, 2}, {3, 4}};
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(20, buffer.data.size());
    auto actual_values = cassandra::io::ReadBuffer<Map>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

UTEST(MapTypesIO, StringUnorderedMap) {
    cassandra::io::protocol::Buffer buffer;

    using Map = cassandra::io::StringMap;
    Map expected_values = {
        {cassandra::io::String("ABC"), cassandra::io::String("GHJ")},
        {cassandra::io::String("DEF"), cassandra::io::String("KLM")}
    };
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(22, buffer.data.size());
    auto actual_values = cassandra::io::ReadBuffer<Map>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

UTEST(MapTypesIO, StringAssociativeMap) {
    cassandra::io::protocol::Buffer buffer;

    using Map = std::map<cassandra::io::String, cassandra::io::String>;
    Map expected_values = {
        {cassandra::io::String("ABC"), cassandra::io::String("GHJ")},
        {cassandra::io::String("DEF"), cassandra::io::String("KLM")}
    };
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(24, buffer.data.size());
    auto actual_values = cassandra::io::ReadBuffer<Map>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

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
