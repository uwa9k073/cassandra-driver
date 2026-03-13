#pragma once

#include <lz4.h>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <memory>

namespace cassandra::io::protocol {
class Lz4Compressor {
public:
    Lz4Compressor() = default;
    ~Lz4Compressor() = default;
    Int Compress(RawBufferView source, RawBuffer& output);
    RawBuffer Decompress(RawBufferView input);
};

using CompressorPtr = std::unique_ptr<Lz4Compressor>;
}  // namespace cassandra::io::protocol
