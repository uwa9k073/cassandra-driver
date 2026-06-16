#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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
#include <cassandra/detail/query_parameters.hpp>

UTEST(QueryParams, StaticParamsEmpty){
    cassandra::detail::StaticQueryParameters<0> params;

    EXPECT_EQ(params.Size(), 0);
}

UTEST(QueryParams, StaticParamsNonEmpty){
    cassandra::detail::StaticQueryParameters<2> params;

    params.Write(10, std::string{"name"});

    EXPECT_EQ(params.Size(), 2);
}


UTEST(QueryParams, DynamicParamsEmpty){
    cassandra::detail::DynamicQueryParameters params;

    EXPECT_EQ(params.Size(), 0);
}

UTEST(QueryParams, DynamicParamsNonEmpty){
    cassandra::detail::DynamicQueryParameters params;

    params.Write(10, std::string{"name"}, 10, std::string{"name"}, 10, std::string{"name"});

    EXPECT_EQ(params.Size(), 6);
}