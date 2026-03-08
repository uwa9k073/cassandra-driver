#include "connection_impl.hpp"
#include <netinet/tcp.h>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/node_description.hpp>
#include <userver/clients/dns/common.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

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
    // CONFIGURE COMPRESSION
    // SEND STARTUP
    // PERFORM AUTHENTICATION
}

void ConnectionImpl::SendMessage(io::protocol::RequestMessage&& message) {
    std::string buffer;
    message.Serialize(buffer);

    auto* data = reinterpret_cast<void*>(buffer.data());
    size_t size = buffer.length();
    auto returned_len =_socket.SendAll(data, size, userver::engine::Deadline{});
    if(returned_len!=size){
        LOG_ERROR("Failed to send message");
    }
}

void ConnectionImpl::WaitForResult() {

    // ResponseMessage message;
    // Implementation of WaitForResult
    // RECEIVE and Deserialize header
    // RECEIVE and Deserialize body
    // PROCESS RESPONSE
}

}  // namespace cassandra::detail
