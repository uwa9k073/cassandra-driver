#include <detail/stream_pool.hpp>
#include <userver/logging/log.hpp>

namespace cassandra::detail {

std::int16_t StreamPool::Acquire(userver::engine::Deadline deadline) {
    userver::engine::SemaphoreLock lock(_semaphore, deadline);
    if (!lock) throw std::runtime_error("StreamPool exhausted");
    lock.Release();
    return _next_id.fetch_add(1, std::memory_order_relaxed) % kMaxStreams;
}

void StreamPool::Release(std::int16_t) { _semaphore.unlock_shared(); }

}  // namespace cassandra::detail
