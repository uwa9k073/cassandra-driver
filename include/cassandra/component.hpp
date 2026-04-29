#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/database_fwd.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

namespace components {

/// @ingroup userver_components
///
/// @brief Apache Cassandra client component.
///
/// Manages the lifecycle of a Cassandra cluster connection pool and exposes
/// a thread-safe `cassandra::Session` to all dependent components and
/// handlers.
///
/// ## Static configuration
///
/// The component is configured via the static YAML file under
/// `components_manager.components.<component-name>`:
///
/// ```yaml
/// cassandra-component:
///   keyspace: my_keyspace                 # required
///   blocking_task_processor: fs-task-processor
///   dns_resolver: async                   # async | getaddrinfo
///   min-pool-size: 5
///   max-pool-size: 100
///   sync-start: false
///   persistent-prepared-statements: true
///   max-ttl-sec: 3600
///   prepared-statement-cache-ways: 16
///   prepared-statement-cache-way-size: 200
///   max-queue-size: 1000
/// ```
///
/// | Parameter | Type | Default | Description |
/// |-----------|------|---------|-------------|
/// | `keyspace` | string | **required** | Cassandra keyspace for all queries |
/// | `blocking_task_processor` | string | current blocking processor | Task
/// processor for blocking I/O (DNS, connection setup) | | `dns_resolver` | string |
/// `async` | `async` or `getaddrinfo` | | `min-pool-size` | integer | 4 |
/// Connections kept open to each node at all times | | `max-pool-size` | integer |
/// 15 | Maximum connections to each node | | `sync-start` | boolean | false |
/// Establish connections synchronously during startup | |
/// `persistent-prepared-statements` | boolean | true | Cache prepared statements for
/// reuse | | `max-ttl-sec` | integer | none | Connection lifetime in seconds before
/// recycling | | `prepared-statement-cache-ways` | integer | 16 | NWayLRU cache
/// lanes for prepared statements | | `prepared-statement-cache-way-size` | integer |
/// 200 | Entries per cache lane | | `max-queue-size` | integer | 200 | Max clients
/// waiting for a connection before `PoolError` is thrown |
///
/// ## Secure configuration (secdist)
///
/// Cluster node addresses and credentials are loaded from the secdist JSON
/// file (see `userver::components::Secdist`). The expected structure is:
///
/// ```json
/// {
///     "cassandra_settings": {
///         "keyspaces": {
///             "my_keyspace": {
///                 "nodes": [
///                     {
///                         "use-ssl": false,
///                         "use-compression": false,
///                         "allow-all": true,
///                         "contact-point": "127.0.0.1",
///                         "port": 9042
///                     }
///                 ]
///             }
///         }
///     }
/// }
/// ```
///
/// When `allow-all` is `false` an `auth` object with `username` and
/// `password` fields must be present.
///
/// ## Usage example
///
/// Register the component in `main()`:
/// ```cpp
/// component_list
///     .Append<userver::components::Secdist>()
///     .Append<userver::components::DefaultSecdistProvider>()
///     .Append<userver::clients::dns::Component>()
///     .Append<components::Cassandra>("cassandra-component");
/// ```
///
/// Retrieve the session in a handler or another component:
/// ```cpp
/// cassandra::SessionPtr session =
///     context.FindComponent<components::Cassandra>("cassandra-component")
///            .GetSessionPtr();
/// ```
///
/// @see cassandra::Session for the query execution interface
/// @see @ref docs/tutorial/component.md for a full configuration guide
/// @see @ref docs/tutorial/session.md for executing queries
class Cassandra final : public userver::components::ComponentBase {
public:
    /// @brief Constructs and initialises the Cassandra component.
    ///
    /// Reads the keyspace name from `config`, loads cluster node descriptions
    /// from secdist, sets up the blocking task processor, and creates the
    /// internal `cassandra::Session` with the configured pool and connection
    /// settings.
    ///
    /// @param config Static YAML configuration for this component instance.
    ///               Must contain at least the `keyspace` key.
    /// @param context Component context used to resolve dependencies
    ///                (`Secdist`, `StatisticsStorage`, DNS resolver, task
    ///                processors).
    ///
    /// @throws userver::components::ComponentsLoadCancelledException if the
    ///         service is shutting down during startup.
    /// @throws userver::storages::secdist::SecdistError if the secdist entry
    ///         for the specified keyspace is missing or malformed.
    Cassandra(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    /// @brief Returns a shared pointer to the underlying Cassandra session.
    ///
    /// The returned `SessionPtr` is safe to store and use from any thread.
    /// The session remains valid for the lifetime of this component; callers
    /// do not need to re-acquire it on every request.
    ///
    /// @return A `std::shared_ptr<cassandra::Session>` that can be used to
    ///         execute CQL queries via `Session::Execute` and
    ///         `Session::BatchExecute`.
    ///
    /// @see cassandra::Session::Execute
    /// @see cassandra::Session::BatchExecute
    cassandra::SessionPtr GetSessionPtr() const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    /// @brief Opaque handle to the database object that owns the session.
    ///
    /// Holds the `cassandra::Session` created during construction. Using a
    /// shared pointer here allows `GetSessionPtr()` to hand out references
    /// with independent lifetimes without exposing the full `Database` type
    /// in this header.
    cassandra::DatabasePtr _database;
};
}  // namespace components
