#pragma once

#include <lz4.h>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <memory>

namespace cassandra::io::protocol {
    
class Compressor {
public:
    virtual ~Compressor() = default;
    virtual Int Compress(RawBufferView source, RawBuffer& output) = 0;
    virtual RawBuffer Decompress(RawBufferView input) = 0;
};
class Lz4Compressor : public Compressor {
public:
    Lz4Compressor() = default;
    ~Lz4Compressor() = default;
    Int Compress(RawBufferView source, RawBuffer& output) override;
    RawBuffer Decompress(RawBufferView input) override;
};

using CompressorPtr = std::unique_ptr<Compressor>;
}  // namespace cassandra::io::protocol
