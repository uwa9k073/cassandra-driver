#include "connection_stream.hpp"

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
    return _consumer.PopNoblock(value);
}
}  // namespace cassandra::detail
