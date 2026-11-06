#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/inet.hpp>
#include <cassandra/io/integral_types.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/ip.hpp>

UTEST(InetIO, IpV4RoundTrip) {
    auto ipv4_expected = userver::utils::ip::AddressV4FromString("255.255.255.255");

    cassandra::io::Bytes buffer;

    cassandra::io::WriteBuffer(buffer, ipv4_expected);

    EXPECT_EQ(buffer.payload.index(), 1);

    auto payload = std::get<1>(buffer.payload);

    EXPECT_EQ(payload.size(), ipv4_expected.kAddressSize);

    auto ipv4_actual = cassandra::io::ReadBuffer<cassandra::io::InetV4>(buffer);

    EXPECT_EQ(ipv4_expected, ipv4_actual);
}

UTEST(InetIO, IpV6RoundTrip) {
    auto ipv6_expected = userver::utils::ip::AddressV6FromString("ffff::");
    cassandra::io::Bytes buffer;

    cassandra::io::WriteBuffer(buffer, ipv6_expected);

    EXPECT_EQ(buffer.payload.index(), 1);

    auto payload = std::get<1>(buffer.payload);

    EXPECT_EQ(payload.size(), ipv6_expected.kAddressSize);

    auto ipv6_actual = cassandra::io::ReadBuffer<cassandra::io::InetV6>(buffer);

    EXPECT_EQ(ipv6_expected, ipv6_actual);
}
