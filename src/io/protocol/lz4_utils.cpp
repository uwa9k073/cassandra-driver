#include <fmt/format.h>
#include <lz4.h>
#include <boost/endian/conversion.hpp>
#include <cassandra/exception.hpp>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstring>
#include <userver/logging/log.hpp>
#include "cassandra/io/integral_types.hpp"

namespace cassandra::io::protocol {

Int Lz4Compressor::Compress(RawBufferView source, RawBuffer& output) {
    size_t source_size = source.size();

    size_t output_size = output.size();

    auto compressed_size = LZ4_compressBound(source_size);

    output.resize(output_size + compressed_size + sizeof(Int));

    Int ret = LZ4_compress_default(
        reinterpret_cast<const char*>(source.data()),
        reinterpret_cast<char*>(output.data() + output_size + sizeof(Int)),
        source_size,
        compressed_size
    );

    if (ret <= 0) [[unlikely]] {
        throw exceptions::Error("LZ4 compression failed");
    }

    detail::WriteIntBE(
        output.data() + output_size,
        output.data() + output_size + sizeof(Int),
        static_cast<Int>(source_size)
    );

    output.resize(output_size + sizeof(Int) + ret);

    return source_size;
}

RawBuffer Lz4Compressor::Decompress(RawBufferView source) {
    size_t offset = 0;
    auto uncompressed_size = detail::ReadIntBE<Int>(source, offset);

    auto compressed_buffer = source.subspan(offset, source.size() - offset);

    RawBuffer output;
    output.resize(uncompressed_size);
    int ret = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed_buffer.data()),
        reinterpret_cast<char*>(output.data()),
        compressed_buffer.size(),
        uncompressed_size
    );
    if (ret != uncompressed_size) [[unlikely]] {
        throw exceptions::Error(fmt::format(
            "LZ4 decompression failed, expected: {}, actual size: {}",
            uncompressed_size,
            ret
        ));
    }
    return output;
}

}  // namespace cassandra::io::protocol
