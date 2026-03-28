#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <detail/connection.hpp>

#include <cassandra/exception.hpp>
#include <cassandra/node_description.hpp>
#include <detail/connection_impl.hpp>
#include <exception>
#include <memory>
#include <string>
#include <userver/clients/dns/common.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/utils/zstring_view.hpp>
#include "cassandra/options.hpp"
#include "cassandra/result_set.hpp"

namespace cassandra::detail {
namespace {

constexpr TimeoutDuration kMinConnectTimeout = std::chrono::seconds{2};

userver::clients::dns::AddrVector ResolveContactPoint(
    const ContactPoint& contact_point,
    userver::clients::dns::Resolver& resolver,
    userver::engine::Deadline deadline
) {
    return resolver.Resolve(contact_point.GetUnderlying(), deadline);
}
struct AddrinfoDeleter {
    void operator()(struct addrinfo* p) const noexcept { freeaddrinfo(p); }
};
using AddrinfoPtr = std::unique_ptr<struct addrinfo, AddrinfoDeleter>;

userver::clients::dns::AddrVector GetAddrInfo(
    ContactPoint contact_point, Port port
) {
    struct addrinfo hints {
    }, *ai_result_raw = nullptr;

    hints.ai_family =
        static_cast<int>(userver::engine::io::AddrDomain::kUnspecified);
    hints.ai_socktype = SOCK_STREAM;

    userver::utils::zstring_view port_string = std::to_string(port.GetUnderlying());

    if (getaddrinfo(
            contact_point.GetUnderlying().c_str(),
            port_string.c_str(),
            &hints,
            &ai_result_raw
        )) {
        LOG_DEBUG("Unknown Host: {}", contact_point.GetUnderlying());
        return {};
    }

    userver::clients::dns::AddrVector result;
    const AddrinfoPtr ai_result(ai_result_raw);
    for (auto* res = ai_result.get(); res; res = res->ai_next) {
        const userver::engine::io::Sockaddr current_addr(res->ai_addr);
        result.push_back(current_addr);
    }
    return result;
}

userver::clients::dns::AddrVector TryResolveContactPoint(
    const ContactPoint& contact_point,
    const Port& port,
    userver::clients::dns::Resolver* resolver,
    userver::engine::Deadline deadline
) {
    userver::clients::dns::AddrVector result;
    if (resolver) {
        try {
            result = ResolveContactPoint(contact_point, *resolver, deadline);
        } catch (const std::exception& e) {
            throw exceptions::ConnectionError{e.what()};
        }
    } else {
        result = GetAddrInfo(contact_point, port);
    }

    if (result.empty()) {
        throw exceptions::ConnectionError("Unknown host");
    }

    for (auto& addr : result) {
        addr.SetPort(port.GetUnderlying());
    }

    return result;
}
}  // namespace

Connection::Connection() = default;

Connection::~Connection() = default;

std::unique_ptr<Connection> Connection::Connect(
    NodeDescription description,
    userver::clients::dns::Resolver* resolver,
    userver::engine::TaskProcessor& bg_task_processor,
    userver::concurrent::BackgroundTaskStorageCore& bg_task_storage,
    ConnectionSettings settings,
    userver::engine::SemaphoreLock&& size_lock,
    userver::utils::statistics::MetricsStoragePtr metrics
) {
    const auto deadline =
        userver::engine::Deadline::FromDuration(kMinConnectTimeout);
    std::unique_ptr<Connection> conn(new Connection());

    conn->_pimpl = std::make_unique<ConnectionImpl>(
        bg_task_processor,
        bg_task_storage,
        settings,
        std::move(size_lock),
        std::move(metrics)
    );

    auto resolved = TryResolveContactPoint(
        description.contact_point, description.port, resolver, deadline
    );
    conn->_pimpl->AsyncConnect(resolved, description.use_compression, deadline);

    return conn;
}

bool Connection::IsExpired() const { return _pimpl->IsExpired(); }

bool Connection::IsBroken() const { return _pimpl->IsBroken(); }

ResultSet Connection::Execute(
    Consistency level,
    const Query& query,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    return _pimpl->Execute(level, query, params, statement_cmd_ctl);
}

ResultSet Connection::ExecutePrepared(
    Consistency level,
    const io::ShortBytes& statement_id,
    const QueryParameters& params,
    OptionalCommandControl statement_cmd_ctl
) {
    return _pimpl->ExecutePrepared(level, statement_id, params, statement_cmd_ctl);
}

io::ShortBytes Connection::Prepare(const io::LongString& query_name) {
    return _pimpl->Prepare(query_name);
}

ResultSet Connection::BatchExecute(
    Consistency level,
    const std::vector<BatchStatement>& store,
    OptionalCommandControl statement_cmd_ctl
) {
    return _pimpl->BatchExecute(level, std::move(store), statement_cmd_ctl);
}
}  // namespace cassandra::detail
