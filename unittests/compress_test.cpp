#include <gtest/gtest.h>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include "cassandra/io/buffer_reader.hpp"
#include "cassandra/io/buffer_writer.hpp"
#include "cassandra/io/cassandra_types.hpp"
#include "cassandra/io/protocol/types.hpp"

TEST(Lz4Compressor, Compress) {
    cassandra::io::String input("Hello, World!");
    cassandra::io::protocol::RawBuffer input_buffer, output_buffer;
    cassandra::io::BufferWriter writer(input_buffer);

    writer.Write(input);
    cassandra::io::protocol::Lz4Compressor compressor;
    auto ret = compressor.Compress(input_buffer, output_buffer);
    cassandra::io::BufferReader<cassandra::io::protocol::BufferView> reader(
        output_buffer
    );
    auto compressed_size = reader.Read<cassandra::io::Int>();
    EXPECT_EQ(compressed_size, ret);
    EXPECT_EQ(compressed_size, input_buffer.size());
}

TEST(Lz4Compressor, Decompress) {
    cassandra::io::String input("Hello, World!");
    cassandra::io::protocol::RawBuffer input_buffer, output_buffer;
    cassandra::io::BufferWriter writer(input_buffer);

    writer.Write(input);
    cassandra::io::protocol::Lz4Compressor compressor;
    compressor.Compress(input_buffer, output_buffer);
    auto decompressed = compressor.Decompress(output_buffer);

    cassandra::io::BufferReader<cassandra::io::protocol::BufferView> reader(
        decompressed
    );
    EXPECT_EQ(input_buffer, decompressed);
    cassandra::io::String decompressed_str = reader.Read<cassandra::io::String>();
    EXPECT_EQ(decompressed_str, input);
}
