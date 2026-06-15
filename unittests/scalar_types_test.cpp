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

// Scalar Types Tests (Int, Float, Double)
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