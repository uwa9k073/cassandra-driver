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
    auto message = WaitForResult();

    auto options = std::dynamic_pointer_cast<io::protocol::SupportMessage>(message)->GetOptions();

    auto json = userver::formats::json::ValueBuilder{options}.ExtractValue();

    LOG_DEBUG("CASSANDRA OPTIONS: {}", userver::formats::json::ToString(json));
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
    auto direction = static_cast<io::protocol::MessageDirection>(header.version);
    if (direction != io::protocol::MessageDirection::kResponse) {
        LOG_ERROR("Unexpected message direction");
        return nullptr;
    }
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
    auto ReadExact = [this](std::vector<char>& buf, size_t size, auto duration) {
        size_t total_read = 0;
        while (total_read < size) {
            auto len = _socket.RecvSome(
                buf.data() + total_read, size - total_read, userver::engine::Deadline::FromDuration(duration)
            );

            if (IsExpired()) {
                throw std::runtime_error("Connection expired");
            }

            if (len <= 0) {
                LOG_ERROR() << "Socket closed or timeout. Read: " << total_read << "/" << size;
                throw std::runtime_error("Socket read failed");
            }
            total_read += len;
            LOG_DEBUG() << "Read chunk: " << len << " bytes, total: " << total_read << "/" << size;
        }
    };

    constexpr size_t kHeaderSize = io::protocol::FrameHeader::kHeaderSize;
    std::vector<char> header_buffer(kHeaderSize);

    ReadExact(header_buffer, kHeaderSize, std::chrono::seconds{15});

    auto header =
        io::protocol::ResponseMessage::DeserializeHeader(std::string_view(header_buffer.data(), header_buffer.size()));
    auto message = GetResponseMessageFromHeader(std::move(header));
    LOG_DEBUG("MESSAGE NOT EMPTY: {}", message != nullptr);

    LOG_DEBUG() << "Received cassandra message opcode: " << static_cast<uint8_t>(message->GetOpcode());
    LOG_DEBUG() << "Received cassandra body len: " << message->GetHeader().length;
    auto body_length = message->GetHeader().length;
    std::vector<char> body_buffer(body_length);

    ReadExact(body_buffer, body_length, std::chrono::seconds{15});

    LOG_DEBUG() << "BUFFER SIZE: " << body_buffer.size();  // Will now correctly print 102
    message->DeserializeBody(body_buffer);
    return message;
}

}  // namespace cassandra::detail
