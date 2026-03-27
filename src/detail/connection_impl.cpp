#include <fmt/format.h>
#include <netinet/tcp.h>
#include <algorithm>
#include <atomic>
#include <cassandra/exception.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/node_description.hpp>
#include <cassandra/options.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <detail/connection_impl.hpp>
#include <detail/stream_pool.hpp>
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
#include <userver/engine/task/cancel.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/trivial_map.hpp>
#include <utility>

namespace cassandra::detail {

ConnectionImpl::ConnectionImpl(
    userver::engine::TaskProcessor& tp,
    userver::concurrent::BackgroundTaskStorageCore& bts,
    ConnectionSettings settings,
    userver::engine::SemaphoreLock&& pool_size_lock,
    userver::utils::statistics::MetricsStoragePtr metrics
)
    : bg_task_processor_(tp),
      bg_task_storage_(bts),
      settings_(settings),
      pool_size_lock_(std::move(pool_size_lock)),
      _metrics(std::move(metrics)),
      _stream_pool(),
      _broken(false) {
    _received_message_queue_map.reserve(StreamPool::kMaxStreams);
    _received_message_producer_map.reserve(StreamPool::kMaxStreams);
    _received_message_consumer_map.reserve(StreamPool::kMaxStreams);

    for (size_t i = 0; i < StreamPool::kMaxStreams; ++i) {
        _received_message_queue_map.emplace_back(RecvMessageQueue::Create(10));
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
        [socket = std::move(tmp_sock),
         sl = std::move(pool_size_lock_),
         reader_loop = std::move(_receiver_task)]() mutable {
            if (reader_loop.IsValid()) {
                reader_loop.RequestCancel();
                reader_loop.Wait();
            }
            if (socket) {
                int _ = std::move(socket).Release();
            }
        }
    );
}

std::shared_ptr<io::protocol::ResponseMessage> ConnectionImpl::ExecuteMessageAsync(
    std::unique_ptr<io::protocol::RequestMessage> message,
    userver::engine::Deadline deadline
) {
    StreamGuard guard(_stream_pool);
    message->SetStreamId(guard.GetStreamId());

    SendMessage(std::move(message), deadline);
    std::shared_ptr<io::protocol::ResponseMessage> result;
    if (!_received_message_consumer_map[guard.GetStreamId()].Pop(result, deadline)) {
        MarkBroken();
        throw std::runtime_error("Timeout");
    }

    return result;
}

std::shared_ptr<io::protocol::ResponseMessage> ConnectionImpl::ExecuteMessage(
    io::protocol::RequestMessage&& message, userver::engine::Deadline deadline
) {
    SendMessage(std::move(message), deadline);

    return ReadFrame(deadline);
}

void ConnectionImpl::StartReceiverLoop() {
    _receiver_task = userver::engine::CriticalAsyncNoSpan(
        bg_task_processor_, [this]() { ReceiverLoop(); }
    );
}

void ConnectionImpl::CqlHandshake(bool use_compression) {
    auto support_message = ExecuteMessage(
        io::protocol::OptionsMessage{},
        userver::engine::Deadline::FromDuration(std::chrono::seconds{1})
    );

    auto options = dynamic_cast<io::protocol::SupportMessage*>(support_message.get())
                       ->GetOptions();

    auto compression_options = options.at(io::String("COMPRESSION"));

    std::shared_ptr<io::protocol::ResponseMessage> message;
    if (use_compression &&
        std::ranges::find(compression_options, io::String("lz4")) !=
            compression_options.end()) {
        _compressor_ptr = std::make_unique<io::protocol::Lz4Compressor>();
        message = ExecuteMessage(
            io::protocol::StartupMessage{"lz4"},
            userver::engine::Deadline::FromDuration(std::chrono::seconds{10})
        );
    } else {
        message = ExecuteMessage(
            io::protocol::StartupMessage{},
            userver::engine::Deadline::FromDuration(std::chrono::seconds{10})
        );
    }

    if (message.get()->GetOpcode() != io::protocol::Opcode::kReady) {
        LOG_ERROR("RECEIVED UNEXPECTED MESSAGE");
        MarkBroken();
        throw std::runtime_error("Unexpected message received");
    }
}

void ConnectionImpl::TcpConnect(
    const userver::clients::dns::AddrVector& addresses,
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
        throw std::runtime_error("Socket not ready");
    }
    LOG_DEBUG("CASSANDRA SOCKET READY");
}

void ConnectionImpl::AsyncConnect(
    userver::clients::dns::AddrVector addresses,
    bool use_compression,
    userver::engine::Deadline deadline
) {
    TcpConnect(addresses, deadline);

    CqlHandshake(use_compression);

    StartReceiverLoop();
}

void ConnectionImpl::SendMessage(
    io::protocol::RequestMessage&& message, userver::engine::Deadline deadline
) {
    io::protocol::RawBuffer buffer;
    message.Serialize(buffer);

    auto lock = std::unique_lock(_send_mutex);

    auto returned_len = _socket.SendAll(buffer.data(), buffer.size(), deadline);
    if (returned_len != buffer.size()) {
        LOG_ERROR("Failed to send message");
    }
}

void ConnectionImpl::SendMessage(
    std::unique_ptr<io::protocol::RequestMessage> message,
    userver::engine::Deadline deadline
) {
    io::protocol::RawBuffer buffer;
    message->Serialize(buffer);

    auto lock = std::unique_lock(_send_mutex);

    auto returned_len = _socket.SendAll(buffer.data(), buffer.size(), deadline);
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

std::shared_ptr<io::protocol::ResponseMessage> ConnectionImpl::ReadFrame(
    userver::engine::Deadline deadline
) {
    constexpr size_t kHeaderSize = io::protocol::FrameHeader::kHeaderSize;

    io::protocol::RawBuffer header_buffer(kHeaderSize);

    auto len = _socket.RecvAll(header_buffer.data(), kHeaderSize, deadline);
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

    len = _socket.RecvAll(body_buffer.data(), body_length, deadline);
    if (len < body_length) {
        LOG_ERROR() << "Socket closed or timeout";
        throw std::runtime_error("Socket read failed");
    }

    LOG_DEBUG() << "BUFFER SIZE: "
                << body_buffer.size();  // Will now correctly print 102
    message->ParseBody(body_buffer);

    if (message->GetOpcode() == io::protocol::Opcode::kError) {
        auto* error_message =
            dynamic_cast<io::protocol::ErrorMessage*>(message.get());
        LOG_WARNING(
            "RECEIVED ERROR MESSAGE: code={}, message={}",
            error_message->GetErrorCode(),
            error_message->GetErrorMessage()
        );
    }

    return message;
}

void ConnectionImpl::MarkBroken() { _broken.store(true, std::memory_order_release); }

void ConnectionImpl::ReceiverLoop() {
    const auto deadline = userver::engine::Deadline{};

    while (!userver::engine::current_task::ShouldCancel()) {
        std::shared_ptr<io::protocol::ResponseMessage> frame;
        try {
            frame = ReadFrame(deadline);
        } catch (const userver::engine::io::IoException& e) {
            LOG_DEBUG() << "ReaderLoop: socket closed, exiting";
            MarkBroken();
            break;
        } catch (std::exception& e) {
            LOG_WARNING("ReaderLoop: read error: {}", e.what());
            MarkBroken();
            break;
        }

        const auto stream_id = frame->GetStreamId();
        if (stream_id < 0) {
            // error handling
            break;
        }

        const auto push_deadline =
            userver::engine::Deadline::FromDuration(std::chrono::seconds{30});
        if (!_received_message_producer_map[stream_id].Push(
                std::move(frame), push_deadline
            )) {
            // Error handling
            break;
        }
    }
    LOG_DEBUG("LOOP EXIT: IsBroken={}", IsBroken());
}

ResultSet ConnectionImpl::Execute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    auto statement_command_control = statement_cmd_ctl.value_or(CommandControl{
        std::chrono::seconds{3},
        std::chrono::seconds{1},
        CommandControl::PreparedStatementsOptionOverride::kNoOverride
    });

    const auto deadline = userver::engine::Deadline::FromDuration(
        statement_command_control.network_timeout_ms
    );  // лучше получить из statement_cmd_ctl

    auto response = ExecuteMessageAsync(
        std::make_unique<io::protocol::QueryMessage>(
            level, query.GetStatement(), params
        ),
        deadline
    );

    auto* result = dynamic_cast<io::protocol::ResultMessage*>(response.get());
    if (!result) throw std::runtime_error("Unexpected response type");

    return result->GetResultSet();
}

ResultSet ConnectionImpl::ExecutePrepared(
    Consistency level,
    const io::ShortBytes& statement_id,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    auto statement_command_control = statement_cmd_ctl.value_or(CommandControl{
        std::chrono::seconds{3},
        std::chrono::seconds{1},
        CommandControl::PreparedStatementsOptionOverride::kNoOverride
    });

    const auto deadline = userver::engine::Deadline::FromDuration(
        statement_command_control.network_timeout_ms
    );

    auto response = ExecuteMessageAsync(
        std::make_unique<io::protocol::ExecuteMessage>(level, statement_id, params),
        deadline
    );

    if (response->GetOpcode() == io::protocol::Opcode::kError) {
        auto* error = dynamic_cast<io::protocol::ErrorMessage*>(response.get());
        throw exceptions::FrameError(
            error->GetErrorCode(), error->GetErrorMessage().GetUnderlying()
        );
    }

    auto* result = dynamic_cast<io::protocol::ResultMessage*>(response.get());
    if (!result) throw std::runtime_error("Unexpected response type");

    return result->GetResultSet();
}

io::ShortBytes ConnectionImpl::Prepare(const io::LongString& query_name) {
    const auto deadline =
        userver::engine::Deadline::FromDuration(std::chrono::seconds{3});

    auto response = ExecuteMessageAsync(
        std::make_unique<io::protocol::PrepareMessage>(query_name), deadline
    );

    if (response->GetOpcode() == io::protocol::Opcode::kError) {
        auto* error = dynamic_cast<io::protocol::ErrorMessage*>(response.get());
        throw exceptions::FrameError(
            error->GetErrorCode(), error->GetErrorMessage().GetUnderlying()
        );
    }

    auto* result = dynamic_cast<io::protocol::ResultMessage*>(response.get());
    if (!result) throw std::runtime_error("Unexpected response type");

    return result->GetPreparedStatementId();
}

}  // namespace cassandra::detail
