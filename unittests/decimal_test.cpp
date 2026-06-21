#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/decimal.hpp>
#include <cassandra/io/integral_types.hpp>
#include <userver/utest/utest.hpp>
#include "utils.hpp"

UTEST(DecimalIO, ReadWrite) {
    cassandra::unittests::utils::ReadWriteBytesTest<cassandra::io::Decimal>(
        cassandra::io::Decimal{.scale = 1, .unscaled = cassandra::io::Varint{128}},
        1,
        6
    );
}
