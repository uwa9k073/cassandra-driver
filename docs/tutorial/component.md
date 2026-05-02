# Working with the Cassandra Component

## Overview

The `Cassandra` component is a userver component that manages connections to an Apache Cassandra cluster. It serves as the central integration point between your userver service and Cassandra, handling connection pooling, session management, configuration parsing, and lifecycle management.

The component is registered as `components::Cassandra` and provides thread-safe access to a `Session` object that you can use throughout your application to execute queries.

## Component Registration

To use the Cassandra component in your service, you must register it in your component list:

```cpp
#include <cassandra/component.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::server::handlers::Ping>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::clients::dns::Component>()
        .Append<components::Cassandra>("cassandra-component")
        // ... other components
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}
```

**Important:** The Cassandra component depends on several other components:
- `Secdist` - for loading sensitive configuration (database credentials, nodes)
- `DefaultSecdistProvider` - for providing secdist configuration files
- `DnsClient` - for DNS resolution (if needed)

Ensure these dependencies are registered **before** the Cassandra component in your component list.

## Static Configuration

The Cassandra component is configured via the static YAML configuration file. Here's a typical configuration:

```yaml
components_manager:
  components:
    secdist: {}
    default-secdist-provider:
      config: config/secure_data.json
      config#env: SECDIST_PATH

    cassandra-component:
      keyspace: my_keyspace
      blocking_task_processor: fs-task-processor
      dns_resolver: async
      min-pool-size: 5
      max-pool-size: 100
      sync-start: false
      persistent-prepared-statements: true
      max-ttl-sec: 3600
      prepared-statement-cache-ways: 16
      prepared-statement-cache-way-size: 200
      max-queue-size: 1000
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `keyspace` | string | **required** | The Cassandra keyspace to use for all queries executed through this component |
| `blocking_task_processor` | string | `engine::current_task::GetBlockingTaskProcessor()` | Name of the task processor for background blocking operations like DNS resolution and connection establishment |
| `dns_resolver` | string | `async` | DNS resolver type: `getaddrinfo` (blocking) or `async` (non-blocking) |
| `min-pool-size` | integer | 4 | Number of connections created initially to each Cassandra node |
| `max-pool-size` | integer | 15 | Maximum number of connections to each Cassandra node |
| `sync-start` | boolean | false | If true, establish connections synchronously during component initialization |
| `persistent-prepared-statements` | boolean | true | Cache prepared statements on the client side for reuse |
| `max-ttl-sec` | integer | none | Maximum lifetime for connections in seconds; connections are recycled after this time |
| `prepared-statement-cache-ways` | integer | 16 | Number of ways in the NWayLRU cache for prepared statements |
| `prepared-statement-cache-way-size` | integer | 200 | Number of entries per way in the prepared statement cache |
| `max-queue-size` | integer | 200 | Maximum number of clients waiting for a connection; throws `PoolError` if exceeded |

## Secure Configuration (secdist)

Cassandra cluster node information and credentials are configured in the secure configuration file (`secure_data.json`). This keeps sensitive information separate from the static configuration.

### Example secdist Configuration

```json
{
    "cassandra_settings": {
        "keyspaces": {
            "my_keyspace": {
                "nodes": [
                    {
                        "use-compression": true,
                        "allow-all": true,
                        "contact-point": "cassandra-node-1.example.com",
                        "port": 9042
                    }
                ]
            }
        }
    }
}
```

### Node Configuration Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `use-compression` | boolean | false | Enable frame compression (reduces bandwidth) |
| `allow-all` | boolean | true | If true, no authentication required. If false, `auth` field must be present |
| `auth` | object | null | Authentication credentials (required if `allow-all` is false) |
| `contact-point` | string | 127.0.0.1 | Hostname or IP address of the Cassandra node |
| `port` | integer | 9042 | Port number where Cassandra is listening |

## Accessing the Component

To use the Cassandra component in your handlers or other components, retrieve the session through dependency injection:

```cpp
class MyHandler : public userver::server::handlers::HttpHandlerJsonBase {
public:
    MyHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : userver::server::handlers::HttpHandlerJsonBase(config, context) {
        // Get the session from the Cassandra component
        auto& cassandra_component = 
            context.FindComponent<components::Cassandra>("cassandra-component");
        _session_ptr = cassandra_component.GetSessionPtr();
    }

private:
    cassandra::SessionPtr _session_ptr;
};
```

Or in another component:

```cpp
class MyComponent : public userver::components::ComponentBase {
public:
    MyComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : userver::components::ComponentBase(config, context) {
        auto& cassandra_component = 
            context.FindComponent<components::Cassandra>();
        _session = cassandra_component.GetSessionPtr();
    }

private:
    cassandra::SessionPtr _session;
};
```

## Component Lifecycle

### Initialization

When the component is initialized:

1. **Configuration parsing** - Static and secure configurations are loaded
2. **Cluster discovery** - Node descriptions are loaded from secdist
3. **Connection pool setup** - Initial connections are created (if `sync-start: true`)
4. **Task processor assignment** - The blocking task processor is configured

If `sync-start` is false (recommended for most services), connections are established lazily on first query.

### Shutdown

When the service shuts down:

1. All connections to Cassandra nodes are closed
2. Connection pools are cleaned up
3. Prepared statement cache is cleared
4. Internal resources are released

## Best Practices

### 1. Single Component Instance

Create only **one** Cassandra component per service for each keyspace you need:

```cpp
// GOOD - Single component for the primary keyspace
.Append<components::Cassandra>("cassandra-component")

// If you need multiple keyspaces
.Append<components::Cassandra>("cassandra-primary")
.Append<components::Cassandra>("cassandra-analytics")
```

### 2. Connection Pool Sizing

Configure pool sizes based on your expected concurrency:

```yaml
cassandra-component:
  # For low-traffic services (< 10 concurrent requests)
  min-pool-size: 2
  max-pool-size: 10
  
  # For medium-traffic services (10-100 concurrent requests)
  min-pool-size: 5
  max-pool-size: 50
  
  # For high-traffic services (> 100 concurrent requests)
  min-pool-size: 10
  max-pool-size: 200
```

### 3. Prepared Statement Caching

Keep `persistent-prepared-statements: true` for services with repeated queries:

```yaml
cassandra-component:
  persistent-prepared-statements: true  # Reuse prepared statements
  prepared-statement-cache-ways: 16      # Adjust based on query diversity
  prepared-statement-cache-way-size: 200
```

For services with very few unique queries, reduce cache size. For services with many unique queries, increase these values.

### 4. Connection TTL

Set `max-ttl-sec` to recycle connections periodically:

```yaml
cassandra-component:
  max-ttl-sec: 3600  # Recycle connections after 1 hour
```

This helps prevent stale connections and handles server-side connection limits.

### 5. Blocking Task Processor

Always specify a dedicated blocking task processor:

```yaml
task_processors:
  fs-task-processor:
    worker_threads: 4

components:
  cassandra-component:
    blocking_task_processor: fs-task-processor
```

This ensures DNS resolution and connection establishment don't block your main request processing.

### 6. Error Handling

Always check if you received valid results:

```cpp
cassandra::Query query("SELECT * FROM users WHERE id = ?");
auto result = session->Execute(cassandra::Consistency::kQuorum, query, user_id);

if (result.Empty()) {
    // Handle empty result
    LOG_WARNING("No rows found for user_id={}", user_id);
} else {
    // Process result
    auto user = result.AsSingleRow<User>(cassandra::io::kRowTag);
}
```

### 7. Consistency Levels

Choose appropriate consistency levels for your use case:

```cpp
// For reads that must have the latest data
session->Execute(cassandra::Consistency::kQuorum, query, id);

// For fast reads where some staleness is acceptable
session->Execute(cassandra::Consistency::kOne, query, id);

// For critical writes
session->Execute(cassandra::Consistency::kAll, insert_query, data);
```

## Common Issues and Solutions

### Issue: "Component not found"

**Problem:** `FindComponent<components::Cassandra>()` throws an exception

**Solution:** Ensure the component is registered before components that depend on it, and use the correct component name in the configuration.

### Issue: "Connection timeout"

**Problem:** Queries fail with connection timeout errors

**Solution:** Check the contact points and ports in secdist configuration, ensure Cassandra nodes are reachable, and verify firewall rules.

### Issue: "Pool exhausted"

**Problem:** `PoolError` is thrown when many requests queue up

**Solution:** Increase `max-pool-size` and `max-queue-size`, and check if your handlers are hanging.

### Issue: "Authentication failed"

**Problem:** "Unauthorized" error from Cassandra

**Solution:** Verify `auth.username` and `auth.password` in secdist match the Cassandra user configuration, and ensure `allow-all: false` is set for authenticated nodes.

## See Also

- @ref docs/tutorial/session.md - Detailed guide on using the Session for queries
- @ref docs/tutorial/result_set.md - Working with query results
- @ref docs/tutorial/example_service.md - Complete example service