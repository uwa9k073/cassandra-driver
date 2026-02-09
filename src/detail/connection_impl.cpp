#include "connection_impl.hpp"
#include <netinet/tcp.h>
#include <userver/clients/dns/common.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include "cassandra/node_description.hpp"

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
    // RECEIVE SUPPORT
    // CONFIGURE COMPRESSION
    // SEND STARTUP
    // PERFORM AUTHENTICATION
}


void ConnectionImpl::SendMessage() const {
    // Implementation of SendMessage
}

void ConnectionImpl::WaitForResult() const {
    // Implementation of WaitForResult
}

}  // namespace cassandra::detail
