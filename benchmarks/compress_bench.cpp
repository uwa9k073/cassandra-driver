
#include <benchmark/benchmark.h>
#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <userver/engine/run_standalone.hpp>
#include <vector>

const std::vector<cassandra::io::String> kStringRawData(
    10000, cassandra::io::String("abababbaba")
);
const std::vector<cassandra::io::BigInt> kIntegralRawData(10000, 1111111);

class Lz4Benchmark : public benchmark::Fixture {
public:
    cassandra::io::protocol::RawBuffer input_buffer;
    cassandra::io::protocol::RawBuffer compressed_buffer;
    cassandra::io::protocol::Lz4Compressor compressor;

    static void SetupBenchmark(::benchmark::internal::Benchmark* b) {
        b->Range(1000, 100000000);
    }

    void SetUp(const benchmark::State&) {
        cassandra::io::BufferWriter writer(input_buffer);
        writer.Write(kIntegralRawData);
    }
};

BENCHMARK_F(Lz4Benchmark, BM_Lz4Compress)(benchmark::State& state) {
    cassandra::io::protocol::RawBuffer output_buffer;

    for (auto _ : state) {
        // Reset output buffer size to ensure consistent allocation behavior
        output_buffer.clear();

        // Prevent optimization
        benchmark::DoNotOptimize(compressor.Compress(input_buffer, output_buffer));
    }

    // Report throughput
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(input_buffer.size())
    );
}

BENCHMARK_REGISTER_F(Lz4Benchmark, BM_Lz4Compress)->Unit(benchmark::kMicrosecond);
// BENCHMARK_REGISTER_F(Lz4Benchmark,
// BM_Lz4Decompress)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
