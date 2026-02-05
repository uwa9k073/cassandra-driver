#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>
namespace cassandra::detail {

class StreamPool final {
public:
    static constexpr int16_t kMinClientStreamId = 0;
    static constexpr int16_t kMaxClientStreamId = 32767;  // 2^15 - 1
    static constexpr int16_t kServerStreamId = -1;        // Для EVENT сообщений
    static constexpr size_t kTotalStreams = kMaxClientStreamId + 1;
    static constexpr size_t kDefaultMaxConcurrent = 1024;  // Ограничение по умолчанию

    StreamPool();

    std::int16_t Acquire();

    void Release(std::int16_t id);

private:
    using StreamIdQueue = userver::concurrent::NonFifoMpmcQueue<std::int16_t>;
    using Producer = StreamIdQueue::MultiProducer;
    using Consumer = StreamIdQueue::MultiConsumer;

    std::shared_ptr<StreamIdQueue> _queue;
    Consumer _consumer;
    Producer _producer;
};
}  // namespace cassandra::detail
