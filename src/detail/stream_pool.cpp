#include <atomic>
#include <detail/stream_pool.hpp>
#include <userver/logging/log.hpp>
#include "cassandra/exception.hpp"

namespace cassandra::detail {

std::int16_t StreamPool::Acquire(userver::engine::Deadline deadline) {
    int id;

    if (!_consumer.Pop(id, deadline)) {
        throw ::cassandra::exceptions::ConnectionError("StreamPool exhausted");
    }

    _used_streams.fetch_add(1, std::memory_order_relaxed);

    return id;
}

void StreamPool::Release(std::int16_t id) {
    if (!_producer.PushNoblock(id))
        LOG_WARNING() << "Failed to release stream " << id;
    _used_streams.fetch_sub(1, std::memory_order_relaxed);
}

}  // namespace cassandra::detail
