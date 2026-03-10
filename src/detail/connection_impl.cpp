#include "connection_impl.hpp"
#include <netinet/tcp.h>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/node_description.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <userver/clients/dns/common.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
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
      _metrics(std::move(metrics)) {}

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
void ConnectionImpl::AsyncConnect(userver::clients::dns::AddrVector addresses, userver::engine::Deadline deadline) {
    for (auto addr : addresses) {
        try {
            userver::engine::io::Socket socket{addr.Domain(), userver::engine::io::SocketType::kTcp};
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
    auto support_message = std::dynamic_pointer_cast<io::protocol::SupportMessage>(WaitForResult());

    auto options = support_message->GetOptions();

    auto json = userver::formats::json::ValueBuilder{options}.ExtractValue();

    LOG_DEBUG("CASSANDRA OPTIONS: {}", userver::formats::json::ToString(json));
    // CONFIGURE COMPRESSION
    // SEND STARTUP
    SendMessage(io::protocol::StartupMessage{});

    auto message = WaitForResult();

    if (message->GetOpcode() == io::protocol::Opcode::kReady) {
        LOG_DEBUG("RECEIVED READY MESSAGE");
    } else {
        LOG_ERROR("RECEIVED AUTH MESSAGE");
    }
}

void ConnectionImpl::SendMessage(io::protocol::RequestMessage&& message) {
    io::protocol::RawBuffer buffer;
    message.Serialize(buffer);

    auto returned_len = _socket.SendAll(buffer.data(), buffer.size(), userver::engine::Deadline{});
    if (returned_len != buffer.size()) {
        LOG_ERROR("Failed to send message");
    }
}

std::shared_ptr<io::protocol::ResponseMessage> GetResponseMessageFromHeader(io::protocol::FrameHeader&& header) {
    auto opcode = header.opcode;
    auto direction = static_cast<io::protocol::MessageDirection>(header.version);
    if (direction != io::protocol::MessageDirection::kResponse) {
        LOG_ERROR("Unexpected message direction");
        return nullptr;
    }
    switch (opcode) {
        case io::protocol::Opcode::kSupported:
            return std::make_shared<io::protocol::SupportMessage>(std::move(header));
        case io::protocol::Opcode::kReady:
            return std::make_shared<io::protocol::ReadyMessage>(std::move(header));
        case io::protocol::Opcode::kAuthenticate:
            return std::make_shared<io::protocol::AuthentificateMessage>(std::move(header));
        default:
            return nullptr;
    }
}

std::shared_ptr<io::protocol::ResponseMessage> ConnectionImpl::WaitForResult() {
    constexpr size_t kHeaderSize = io::protocol::FrameHeader::kHeaderSize;
    io::protocol::RawBuffer header_buffer(kHeaderSize);

    auto len = _socket.RecvSome(
        header_buffer.data(), kHeaderSize, userver::engine::Deadline::FromDuration(std::chrono::seconds{15})
    );
    if (len <= 0) {
        LOG_ERROR() << "Socket closed or timeout";
        throw std::runtime_error("Socket read failed");
    }

    auto header = io::protocol::ResponseMessage::ParseHeader(header_buffer);

    auto message = GetResponseMessageFromHeader(std::move(header));
    LOG_DEBUG("MESSAGE NOT EMPTY: {}", message != nullptr);

    LOG_DEBUG() << "Received cassandra message opcode: " << static_cast<uint8_t>(message->GetOpcode());
    LOG_DEBUG() << "Received cassandra body len: " << message->GetHeader().length;

    auto body_length = message->GetHeader().length;
    io::protocol::RawBuffer body_buffer;
    body_buffer.resize(body_length);

    len = _socket.RecvSome(
        body_buffer.data(), body_length, userver::engine::Deadline::FromDuration(std::chrono::seconds{15})
    );
    if (len <= 0) {
        LOG_ERROR() << "Socket closed or timeout";
        throw std::runtime_error("Socket read failed");
    }

    LOG_DEBUG() << "BUFFER SIZE: " << body_buffer.size();  // Will now correctly print 102
    message->ParseBody(body_buffer);
    return message;
}

}  // namespace cassandra::detail
