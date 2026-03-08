#include "connection_impl.hpp"
#include <netinet/tcp.h>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/node_description.hpp>
#include <cstdint>
#include <memory>
#include <userver/clients/dns/common.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
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
    // RECEIVE SUPPORT
    auto message = WaitForResult();
    LOG_DEBUG() << "Received cassandra message opcode: " << static_cast<uint8_t>(message->GetOpcode());
    // CONFIGURE COMPRESSION
    // SEND STARTUP
    // PERFORM AUTHENTICATION
}

void ConnectionImpl::SendMessage(io::protocol::RequestMessage&& message) {
    std::string buffer;
    message.Serialize(buffer);

    auto* data = reinterpret_cast<void*>(buffer.data());
    size_t size = buffer.length();
    auto returned_len = _socket.SendAll(data, size, userver::engine::Deadline{});
    if (returned_len != size) {
        LOG_ERROR("Failed to send message");
    }
}

std::shared_ptr<io::protocol::ResponseMessage> GetResponseMessageFromHeader(io::protocol::FrameHeader&& header) {
    auto opcode = header.opcode;
    switch (opcode) {
        case io::protocol::Opcode::kSupported:
            return std::make_shared<io::protocol::SupportMessage>(std::move(header));
        case io::protocol::Opcode::kError:
        case io::protocol::Opcode::kStartup:
        case io::protocol::Opcode::kReady:
        case io::protocol::Opcode::kAuthenticate:
        case io::protocol::Opcode::kCredentials:
        case io::protocol::Opcode::kOptions:
        case io::protocol::Opcode::kQuery:
        case io::protocol::Opcode::kResult:
        case io::protocol::Opcode::kPrepare:
        case io::protocol::Opcode::kExecute:
        case io::protocol::Opcode::kRegister:
        case io::protocol::Opcode::kEvent:
        case io::protocol::Opcode::kBatch:
        case io::protocol::Opcode::kAuthChallenge:
        case io::protocol::Opcode::kAuthResponse:
        case io::protocol::Opcode::kAuthSuccess:
            return nullptr;
    }
}

std::shared_ptr<io::protocol::ResponseMessage> ConnectionImpl::WaitForResult() {
    std::string buf;
    buf.reserve(io::protocol::FrameHeader::kHeaderSize);
    auto _ = _socket.RecvAll(reinterpret_cast<void*>(buf.data()), buf.capacity(), userver::engine::Deadline{});

    auto header = io::protocol::ResponseMessage::DeserializeHeader(buf);
    buf.clear();
    buf.reserve(header.length);
    _ = _socket.RecvAll(reinterpret_cast<void*>(buf.data()), buf.capacity(), userver::engine::Deadline{});
    // io::protocol::SupportMessage message(std::move(header));
    auto message = GetResponseMessageFromHeader(std::move(header));

    message->DeserializeBody(buf);
    return message;
}

}  // namespace cassandra::detail
