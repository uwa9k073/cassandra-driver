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

// List Types Tests (Vector, StringList, std::list)
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