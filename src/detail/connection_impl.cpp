#include <netinet/tcp.h>
#include <algorithm>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/query.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <detail/connection_impl.hpp>
#include <memory>
#include <userver/clients/dns/common.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <utility>
#include "cassandra/result_set.hpp"
#include "detail/stream_pool.hpp"
namespace cassandra::detail {
ConnectionImpl::ConnectionImpl(
    userver::engine::TaskProcessor& tp,
    userver::concurrent::BackgroundTaskStorageCore& bts,
    ConnectionSettings settings,
    userver::engine::SemaphoreLock&& pool_size_lock,
    userver::utils::statistics::MetricsStoragePtr metrics,
    std::shared_ptr<StreamPool> stream_pool_ptr
)
    : bg_task_processor_(tp),
      bg_task_storage_(bts),
      settings_(settings),
      pool_size_lock_(std::move(pool_size_lock)),
      _metrics(std::move(metrics)),
      _stream_pool_ptr(stream_pool_ptr) {
    _received_message_queue_map.reserve(StreamPool::kTotalStreams);
    _received_message_producer_map.reserve(StreamPool::kTotalStreams);
    _received_message_consumer_map.reserve(StreamPool::kTotalStreams);

    for (size_t i = 0; i < StreamPool::kTotalStreams; ++i) {
        _received_message_queue_map.emplace_back(RecvMessageQueue::Create(100));
        auto back = _received_message_queue_map.back();
        _received_message_producer_map.emplace_back(back->GetProducer());
        _received_message_consumer_map.emplace_back(back->GetConsumer());
    }
}

ConnectionImpl::~ConnectionImpl() { bg_task_storage_.Detach(Close()); }

userver::engine::Task ConnectionImpl::Close() {
    userver::engine::io::Socket tmp_sock = std::exchange(_socket, {});

    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    return userver::engine::CriticalAsyncNoSpan(
        bg_task_processor_,
        [socket = std::move(tmp_sock), sl = std::move(pool_size_lock_)]() mutable {
            if (socket) {
                int _ = std::move(socket).Release();
            }
        }
    );
}
void ConnectionImpl::AsyncConnect(
    userver::clients::dns::AddrVector addresses,
    bool use_compression,
    userver::engine::Deadline deadline
) {
    for (auto addr : addresses) {
        try {
            userver::engine::io::Socket socket{
                addr.Domain(), userver::engine::io::SocketType::kTcp
            };
            socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
            socket.Connect(addr, deadline);
            _socket = std::move(socket);
            break;
        } catch (userver::engine::io::IoException& e) {
            LOG_DEBUG("Cannot connect to {}: ", addr.PrimaryAddressString()) << e;
        }
    }

    if (!_socket.IsValid()) {
        LOG_DEBUG("SOCKET NOT READY");
        return;
    }
    LOG_DEBUG("CASSANDRA SOCKET READY");

    // SENDING OPTIONS
    SendMessage(io::protocol::OptionsMessage{});
    LOG_DEBUG("SENDED OPTIONS MESSAGE");
    // RECEIVE SUPPORT
    auto support_message = ReadFrame();

    auto options =
        reinterpret_cast<io::protocol::SupportMessage*>(support_message.get())
            ->GetOptions();

    auto compression_options = options.at(io::String("COMPRESSION"));

    auto json = userver::formats::json::ValueBuilder{options}.ExtractValue();

    LOG_DEBUG("CASSANDRA OPTIONS: {}", userver::formats::json::ToString(json));
    // CONFIGURE COMPRESSION
    // SEND STARTUP

    if (std::ranges::find(compression_options, io::String("lz4")) !=
            compression_options.end() &&
        use_compression) {
        _compressor_ptr = std::make_unique<io::protocol::Lz4Compressor>();
        SendMessage(io::protocol::StartupMessage{"lz4"});
    } else {
        SendMessage(io::protocol::StartupMessage{});
    }

    auto message = ReadFrame();

    if (message.get()->GetOpcode() == io::protocol::Opcode::kReady) {
        LOG_DEBUG("RECEIVED READY MESSAGE");
    }
}

void ConnectionImpl::SendMessage(io::protocol::RequestMessage&& message) {
    io::protocol::RawBuffer buffer;
    message.Serialize(buffer);

    auto returned_len =
        _socket.SendAll(buffer.data(), buffer.size(), userver::engine::Deadline{});
    if (returned_len != buffer.size()) {
        LOG_ERROR("Failed to send message");
    }
}

std::shared_ptr<io::protocol::ResponseMessage> GetResponseMessageFromHeader(
    io::protocol::FrameHeader&& header
) {
    auto opcode = header.opcode;
    auto direction = static_cast<io::protocol::MessageDirection>(header.version);
    if (direction != io::protocol::MessageDirection::kResponse) {
        LOG_ERROR("Unexpected message direction");
        throw std::runtime_error("Unexpected message direction");
    }
    switch (opcode) {
        case io::protocol::Opcode::kSupported:
            return std::make_shared<io::protocol::SupportMessage>(std::move(header));
        case io::protocol::Opcode::kReady:
            return std::make_shared<io::protocol::ReadyMessage>(std::move(header));
        case io::protocol::Opcode::kAuthenticate:
            return std::make_shared<io::protocol::AuthentificateMessage>(
                std::move(header)
            );
        case io::protocol::Opcode::kError:
            return std::make_shared<io::protocol::ErrorMessage>(std::move(header));
        case io::protocol::Opcode::kResult:
            return std::make_shared<io::protocol::ResultMessage>(std::move(header));
        default:
            throw std::runtime_error("Unexpected opcode");
    }
}

std::shared_ptr<io::protocol::ResponseMessage> ConnectionImpl::ReadFrame() {
    constexpr size_t kHeaderSize = io::protocol::FrameHeader::kHeaderSize;

    io::protocol::RawBuffer header_buffer(kHeaderSize);

    auto len = _socket.RecvSome(
        header_buffer.data(),
        kHeaderSize,
        userver::engine::Deadline::FromDuration(std::chrono::seconds{15})
    );
    if (len <= 0) {
        LOG_ERROR() << "Socket closed or timeout";
        throw std::runtime_error("Socket read failed");
    }

    auto header = io::protocol::ResponseMessage::ParseHeader(header_buffer);

    LOG_DEBUG() << "Received cassandra message opcode: "
                << static_cast<uint8_t>(header.opcode);
    auto message = GetResponseMessageFromHeader(std::move(header));
    LOG_DEBUG("MESSAGE NOT EMPTY: {}", message != nullptr);
    LOG_DEBUG() << "Received cassandra body len: " << message->GetHeader().length;

    size_t body_length = message->GetHeader().length;
    io::protocol::RawBuffer body_buffer;
    body_buffer.resize(body_length);

    len = _socket.RecvSome(
        body_buffer.data(),
        body_length,
        userver::engine::Deadline::FromDuration(std::chrono::seconds{15})
    );
    if (len < body_length) {
        LOG_ERROR() << "Socket closed or timeout";
        throw std::runtime_error("Socket read failed");
    }

    LOG_DEBUG() << "BUFFER SIZE: "
                << body_buffer.size();  // Will now correctly print 102
    message->ParseBody(body_buffer);

    if (message->GetOpcode() == io::protocol::Opcode::kError) {
        auto error_message =
            reinterpret_cast<io::protocol::ErrorMessage*>(message.get());
        LOG_WARNING(
            "RECEIVED ERROR MESSAGE: code={}, message={}",
            error_message->GetErrorCode(),
            error_message->GetErrorMessage()
        );
        throw std::runtime_error("Received error message");
    }

    return message;
}

void ConnectionImpl::WaitForResult(
    MessagePromise&& promise, std::int16_t stream_id
) {
    auto& consumer = _received_message_consumer_map.at(stream_id);
    std::shared_ptr<io::protocol::ResponseMessage> promise_value = nullptr;

    LOG_DEBUG("TRYING TO GET MESSAGE BY STREAM ID");

    while (!consumer.PopNoblock(promise_value)) {
        LOG_DEBUG("POP NOBLOCK FAILED, READING FRAME");
        promise_value = ReadFrame();
        if (promise_value->GetStreamId() == stream_id) {
            promise.set_value(promise_value);
            return;
        } else {
            LOG_DEBUG("PUSHING FRAME TO QUEUE");
            auto& producer = _received_message_producer_map.at(promise_value->GetStreamId());
            [[maybe_unused]] auto _ = producer.PushNoblock(std::move(promise_value));
            userver::engine::Yield();
        }
    }

    LOG_DEBUG("FRAME GETTED");
    promise.set_value(promise_value);
}

class StreamGuard {
public:
    StreamGuard(std::shared_ptr<StreamPool> stream_pool)
        : _stream_pool(stream_pool) {
        _stream_id = _stream_pool->Acquire();
    }

    ~StreamGuard() { _stream_pool->Release(_stream_id); }

    std::int16_t GetStreamId() const { return _stream_id; }

private:
    std::shared_ptr<StreamPool> _stream_pool;
    std::int16_t _stream_id;
};

ResultSet ConnectionImpl::Execute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl /*statement_cmd_ctl*/
) {
    StreamGuard sg(_stream_pool_ptr);
    auto stream_id = sg.GetStreamId();

    io::protocol::QueryMessage message(level, query.GetStatement(), params);

    message.SetStreamId(stream_id);

    MessagePromise promise;
    auto future = promise.get_future();
    auto task = userver::engine::AsyncNoSpan(
        bg_task_processor_,
        [message_promise = std::move(promise),
         request = std::move(message),
         this,
         stream_id]() mutable {
            SendMessage(std::move(request));

            WaitForResult(std::move(message_promise), stream_id);
        }
    );

    bg_task_storage_.Detach(std::move(task));
    auto result_message =
        std::dynamic_pointer_cast<io::protocol::ResultMessage>(future.get());
    if (!result_message) {
        throw std::runtime_error("Result message is null");
    }

    LOG_DEBUG("RESULT MESSAGE CORRECT");

    return result_message->GetResultSet();
}

}  // namespace cassandra::detail
