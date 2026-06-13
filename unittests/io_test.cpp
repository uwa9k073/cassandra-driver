#include <gtest/gtest.h>
#include <list>
#include <map>
#include <unordered_map>
#include <userver/utest/utest.hpp>

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include "cassandra/io/buffer_io.hpp"

TEST(ScalarTypesIO, Integral) {
    cassandra::io::protocol::Buffer buffer;

    int value = 42;
    cassandra::io::WriteBuffer(buffer.data, value);
    EXPECT_EQ(4, buffer.data.size());
    auto actual_value = cassandra::io::ReadBuffer<int>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_value, value);
}

TEST(ScalarTypesIO, Float) {
    cassandra::io::protocol::Buffer buffer;

    float value = 3.14f;
    cassandra::io::WriteBuffer(buffer.data, value);
    EXPECT_EQ(4, buffer.data.size());
    auto actual_value =
        cassandra::io::ReadBuffer<float>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_value, value);
}

TEST(ScalarTypesIO, Double) {
    cassandra::io::protocol::Buffer buffer;

    double value = 3.14;
    cassandra::io::WriteBuffer(buffer.data, value);
    EXPECT_EQ(8, buffer.data.size());
    auto actual_value =
        cassandra::io::ReadBuffer<double>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_value, value);
}

TEST(ListTypesIO, IntegralVector) {
    cassandra::io::protocol::Buffer buffer;

    std::vector<int> expected_values = {1, 2, 3};
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(16, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<std::vector<int>>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

TEST(ListTypesIO, StringVector) {
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

TEST(ListTypesIO, IntegralList) {
    cassandra::io::protocol::Buffer buffer;

    std::list<int> expected_values = {1, 2, 3};
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(16, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<std::list<int>>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

TEST(MapTypesIO, IntegralUnorderedMap) {
    cassandra::io::protocol::Buffer buffer;

    using Map = std::unordered_map<int, int>;
    Map expected_values = {{1, 2}, {3, 4}};
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(20, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<Map>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

TEST(MapTypesIO, StringUnorderedMap) {
    cassandra::io::protocol::Buffer buffer;

    using Map = cassandra::io::StringMap;
    Map expected_values = {
        {cassandra::io::String("ABC"), cassandra::io::String("GHJ")},
        {cassandra::io::String("DEF"), cassandra::io::String("KLM")}
    };
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_EQ(22, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<Map>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}

TEST(MapTypesIO, StringAssociativeMap) {
    cassandra::io::protocol::Buffer buffer;

    using Map = std::map<cassandra::io::String, cassandra::io::String>;
    Map expected_values = {
        {cassandra::io::String("ABC"), cassandra::io::String("GHJ")},
        {cassandra::io::String("DEF"), cassandra::io::String("KLM")}
    };
    cassandra::io::WriteBuffer(buffer.data, expected_values);
    EXPECT_NE(22, buffer.data.size());
    auto actual_values =
        cassandra::io::ReadBuffer<Map>(buffer.data, buffer.offset);
    EXPECT_EQ(actual_values, expected_values);
}
