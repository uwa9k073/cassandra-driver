#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/varint.hpp>
#include <userver/utest/utest.hpp>
#include <vector>
#include "utils.hpp"

UTEST(ParseVarInt, ReadZero) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            cassandra::io::Bytes{.payload = std::vector{std::byte{0x00}}}
        ).GetUnderlying(),
        0
    );
}

UTEST(ParseVarInt, ReadOne) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0x01}}}
        ).GetUnderlying(),
        1
    );
}

UTEST(ParseVarInt, Read127) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0x7f}}}
        ).GetUnderlying(),
        127
    );
}

UTEST(ParseVarInt, Read128) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0x00}, std::byte{0x80}}}
        ).GetUnderlying(),
        128
    );
}

UTEST(ParseVarInt, Read129) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0x00}, std::byte{0x81}}}
        ).GetUnderlying(),
        129
    );
}

UTEST(ParseVarInt, ReadMinusOne) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0xff}}}
        ).GetUnderlying(),
        -1
    );
}

UTEST(ParseVarInt, ReadMinus128) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0x80}}}
        ).GetUnderlying(),
        -128
    );
}

UTEST(ParseVarInt, ReadMinus129) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>(
            {.payload = std::vector{std::byte{0xff}, std::byte{0x7f}}}
        ).GetUnderlying(),
        -129
    );
}

UTEST(ParseVarInt, ReadInt64Max) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>({.payload =
                                                              std::vector{
                                                                  std::byte{0x7f},
                                                                  std::byte{0xff},
                                                                  std::byte{0xff},
                                                                  std::byte{0xff},
                                                                  std::byte{0xff},
                                                                  std::byte{0xff},
                                                                  std::byte{0xff},
                                                                  std::byte{0xff}
                                                              }}
        ).GetUnderlying(),
        INT64_MAX
    );
}

UTEST(ParseVarInt, ReadInt64Min) {
    EXPECT_EQ(
        cassandra::io::ReadBuffer<cassandra::io::Varint>({.payload =
                                                              std::vector{
                                                                  std::byte{0x80},
                                                                  std::byte{0x00},
                                                                  std::byte{0x00},
                                                                  std::byte{0x00},
                                                                  std::byte{0x00},
                                                                  std::byte{0x00},
                                                                  std::byte{0x00},
                                                                  std::byte{0x00}
                                                              }}
        ).GetUnderlying(),
        INT64_MIN
    );
}

UTEST(VarintIO, ReadWrite) {
    cassandra::unittests::utils::ReadWriteBytesTest<cassandra::io::Varint>(
        cassandra::io::Varint{128}, 1, 2
    );
}
