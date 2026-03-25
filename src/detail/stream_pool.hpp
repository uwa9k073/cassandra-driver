#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cstddef>
#include <cstdint>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>

namespace cassandra::detail {
class StreamPool final {
public:
    static constexpr int16_t kMinClientStreamId = 0;
    static constexpr int16_t kMaxClientStreamId = 32767;  // 2^15 - 1
    static constexpr int16_t kServerStreamId = -1;  // Для EVENT сообщений
    static constexpr size_t kTotalStreams = 32768;
    static constexpr size_t kMaxStreams = 1024;  // Ограничение по умолчанию
    StreamPool() : _semaphore(kMaxStreams), _next_id(0) {}

    std::int16_t Acquire(userver::engine::Deadline deadline);

    void Release(std::int16_t id);

private:
    userver::engine::Semaphore _semaphore;
    std::atomic<std::int16_t> _next_id;
};

class StreamGuard {
public:
    StreamGuard(StreamPool& stream_pool) : _stream_pool(stream_pool) {
        _stream_id = _stream_pool.Acquire(
            userver::engine::Deadline::FromDuration(std::chrono::seconds{1})
        );
    }

    ~StreamGuard() { _stream_pool.Release(_stream_id); }

    std::int16_t GetStreamId() const { return _stream_id; }

private:
    StreamPool& _stream_pool;
    std::int16_t _stream_id;
};
}  // namespace cassandra::detail
