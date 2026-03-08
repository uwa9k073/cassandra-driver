#include "stream_pool.hpp"
#include <userver/logging/log.hpp>

namespace cassandra::detail {

StreamPool::StreamPool()
    : _queue(StreamIdQueue::Create()), _consumer(_queue->GetMultiConsumer()), _producer(_queue->GetMultiProducer()) {
    for (size_t i = 0; i < kTotalStreams; ++i) {
        auto value = i;
        [[maybe_unused]] auto _ = _producer.PushNoblock(std::move(value));
    }
}
std::int16_t StreamPool::Acquire() {
    std::int16_t value;
    if (!_consumer.PopNoblock(value)) {
        LOG_WARNING("FAILED TO ACQUIRE STREAM");
        return -2;
    }

    return value;
}

void StreamPool::Release(std::int16_t id) { [[maybe_unused]] auto _ = _producer.PushNoblock(std::move(id)); }

}  // namespace cassandra::detail
