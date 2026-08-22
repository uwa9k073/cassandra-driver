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

// Map Types Tests (UnorderedMap, StringMap, std::map)
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