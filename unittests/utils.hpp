#pragma once

#include <gtest/gtest.h>
#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <concepts>
namespace cassandra::unittests::utils {

template <class Type, class ExpectedValue>
    requires std::convertible_to<ExpectedValue, Type>
void ReadWriteBytesTest(
    const ExpectedValue& expected_value,
    std::size_t expected_index,
    std::optional<std::size_t> expected_size
) {
    Type value = expected_value;
    io::Bytes buffer;

    io::WriteBuffer<Type>(buffer, value);

    EXPECT_EQ(buffer.payload.index(), expected_index);
    if (expected_index && expected_size) {
        EXPECT_EQ(std::get<1>(buffer.payload).size(), expected_size);
    }
    const auto& actual_value = io::ReadBuffer<Type>(buffer);
    EXPECT_EQ(value, actual_value);
}
}  // namespace cassandra::unittests::utils
