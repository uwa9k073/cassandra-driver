#include <gtest/gtest.h>
#include <algorithm>
#include <cassandra/batch_query.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/options.hpp>
#include <vector>

const ::cassandra::Query kInsertQuery{
    "insert into example_ks.example_table (id) values (?)"
};

TEST(BatchStatement, QueryWithParams) {
    cassandra::BatchQueryStore batch(cassandra::Consistency::kLocalOne);
    batch.AddQuery(kInsertQuery, 10, std::string{"name"});

    auto queries = batch.Queries();

    std::vector<cassandra::BatchStatement> statements;
    statements.reserve(queries.size());
    std::ranges::transform(
        queries,
        std::back_inserter(statements),
        [](const cassandra::BatchQuery& el) {
            return cassandra::BatchStatement{
                el.GetQuery().GetStatement(), el.GetParams()
            };
        }
    );

    auto params = statements.front().params;
    EXPECT_FALSE(params.Empty());
    EXPECT_EQ(2, params.Size());

    cassandra::io::BufferReader<cassandra::io::Bytes> reader(params.ParamBuffers()[0]
    );
    ASSERT_EQ(reader.Read<cassandra::io::Int>(), 10);  // stack usage after free here
}
