# Working with the Cassandra Session

The `Session` class is the main interface for executing CQL queries against a Cassandra cluster. It manages connections, connection pooling, query execution, and provides thread-safe access to the database.

## Overview

A Session object:
- Manages a pool of connections to multiple Cassandra nodes
- Executes both single queries and batch operations
- Handles automatic connection pooling and reconnection
- Supports configurable consistency levels
- Provides query preparation and caching
- Is thread-safe and designed to be long-lived

Typically, you create one Session per service during startup (through the Cassandra component) and reuse it across all handlers and components.

## Getting a Session

The Session is obtained from the Cassandra component:

```cpp
#include <cassandra/component.hpp>

class MyHandler : public userver::server::handlers::HttpHandlerJsonBase {
public:
    MyHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : userver::server::handlers::HttpHandlerJsonBase(config, context),
        _session_ptr(context
                         .FindComponent<components::Cassandra>("cassandra-component")
                         .GetSessionPtr()) {}

private:
    cassandra::SessionPtr _session_ptr;
};
```

The returned `SessionPtr` is a `std::shared_ptr<Session>`, making it safe to store and copy.

## Basic Query Execution

### Simple Query with Parameters

Execute a query with parameters using the template Execute method:

```cpp
#include <cassandra/session.hpp>
#include <cassandra/query.hpp>
#include <cassandra/options.hpp>

// Basic query
cassandra::Query insert_query("INSERT INTO users (id, name, email) VALUES (?, ?, ?)");
session->Execute(
    cassandra::Consistency::kQuorum,
    insert_query,
    user_id,
    user_name,
    user_email
);

// Query with single parameter
cassandra::Query select_query("SELECT * FROM users WHERE id = ?");
auto result = session->Execute(
    cassandra::Consistency::kQuorum,
    select_query,
    user_id
);
```

### Query as String View

You can also pass the query as a string view directly:

```cpp
auto result = session->Execute(
    cassandra::Consistency::kQuorum,
    "SELECT * FROM users WHERE id = ? AND active = ?",
    user_id,
    true
);
```

## Consistency Levels

Cassandra provides multiple consistency levels that balance between consistency and performance. Specify the level for each query:

```cpp
#include <cassandra/options.hpp>

// Strong consistency - wait for all replicas
session->Execute(cassandra::Consistency::kAll, query, params...);

// Quorum consistency - wait for majority of replicas
session->Execute(cassandra::Consistency::kQuorum, query, params...);

// Local quorum - wait for quorum within the local data center
session->Execute(cassandra::Consistency::kLocalQuorum, query, params...);

// One - fastest, wait for one replica
session->Execute(cassandra::Consistency::kOne, query, params...);

// Local one - wait for one replica in local data center
session->Execute(cassandra::Consistency::kLocalOne, query, params...);
```

### Choosing Consistency Levels

| Level | Use Case | Trade-offs |
|-------|----------|-----------|
| `kOne` | Fast reads, acceptable staleness | Potential stale data, fast |
| `kLocalOne` | Fast local reads | Same as kOne but local only |
| `kQuorum` | Standard reads/writes | Balanced consistency and performance |
| `kLocalQuorum` | Multi-DC, local consistency | Balanced for local requirements |
| `kAll` | Critical writes requiring strong consistency | Slow, unavailable if any replica down |

## Command Controls

Command controls allow per-query configuration of timeouts and prepared statement handling:

```cpp
#include <cassandra/options.hpp>

// Create a command control
cassandra::CommandControl cmd_ctl(
    std::chrono::milliseconds{5000},  // network timeout
    std::chrono::milliseconds{3000},  // statement timeout
    cassandra::CommandControl::PreparedStatementsOptionOverride::kEnabled
);

// Use it in a query
cassandra::Query query("SELECT * FROM events WHERE id = ?");
auto result = session->Execute(
    cassandra::Consistency::kQuorum,
    cmd_ctl,
    query,
    event_id
);
```

### CommandControl Parameters

- **Network Timeout**: Maximum time to wait for network response
- **Statement Timeout**: Maximum time the statement can execute on the server
- **Prepared Statements Override**: Force enable/disable prepared statement caching for this query

### Example with Different Timeouts

```cpp
// Quick query with tight timeout
cassandra::CommandControl fast_ctl(
    std::chrono::milliseconds{1000},
    std::chrono::milliseconds{500}
);

// Long-running query with loose timeout
cassandra::CommandControl slow_ctl(
    std::chrono::milliseconds{30000},
    std::chrono::milliseconds{20000}
);
```

## Batch Queries

For executing multiple queries as a single atomic batch operation:

```cpp
#include <cassandra/batch_query.hpp>

// Create a batch
cassandra::BatchQueryStore batch(cassandra::Consistency::kQuorum);

// Add queries to the batch
batch.AddQuery(
    "INSERT INTO users (id, name) VALUES (?, ?)",
    1,
    "Alice"
);
batch.AddQuery(
    "INSERT INTO users (id, name) VALUES (?, ?)",
    2,
    "Bob"
);
batch.AddQuery(
    "INSERT INTO users (id, name) VALUES (?, ?)",
    3,
    "Charlie"
);

// Execute the batch
auto result = session->BatchExecute(batch);
```

### Batch with Command Control

```cpp
cassandra::CommandControl batch_ctl(
    std::chrono::milliseconds{10000},
    std::chrono::milliseconds{8000}
);

auto result = session->BatchExecute(batch, batch_ctl);
```

### When to Use Batches

- Inserting/updating many related rows
- Atomic multi-statement operations
- Reducing network round trips

Avoid batches for:
- Queries from different operations (use individual Execute calls)
- Very large batches (> 1000 queries) - split into multiple batches
- When consistency isn't important between queries

## Query Result Handling

All Execute methods return a `ResultSet` object containing query results:

```cpp
auto result = session->Execute(cassandra::Consistency::kQuorum, query, id);

// Check if result is empty
if (result.Empty()) {
    LOG_INFO("No rows found");
    return;
}

// Get metadata
LOG_DEBUG("Rows affected: {}", result.RowsAffected());
LOG_DEBUG("Columns affected: {}", result.ColumnsAffected());

// Extract results
auto user = result.AsSingleRow<User>(cassandra::io::kRowTag);
```

See the @ref docs/tutorial/result_set.md tutorial for detailed information on processing results.

## Exception Handling

Session::Execute methods may throw exceptions in case of errors:

```cpp
#include <cassandra/exceptions.hpp>

try {
    auto result = session->Execute(
        cassandra::Consistency::kQuorum,
        query,
        params...
    );
} catch (const cassandra::exceptions::SessionError& e) {
    LOG_ERROR("Session error: {}", e.what());
    // Handle session establishment failure
} catch (const cassandra::exceptions::ConnectionError& e) {
    LOG_ERROR("Connection error: {}", e.what());
    // Handle connection failure
} catch (const cassandra::exceptions::FrameError& e) {
    LOG_ERROR("Cassandra error: {}", e.what());
    // Handle protocol-level errors
} catch (const std::exception& e) {
    LOG_ERROR("Unexpected error: {}", e.what());
}
```

## Advanced Examples

### Read-Modify-Write Pattern

```cpp
// Read current value
cassandra::Query read_query("SELECT balance FROM accounts WHERE id = ?");
auto read_result = session->Execute(
    cassandra::Consistency::kQuorum,
    read_query,
    account_id
);

if (!read_result.Empty()) {
    int current_balance = read_result.AsSingleRow<int>(cassandra::io::kFieldTag);
    
    // Modify
    int new_balance = current_balance + amount;
    
    // Write back
    cassandra::Query write_query("UPDATE accounts SET balance = ? WHERE id = ?");
    session->Execute(
        cassandra::Consistency::kQuorum,
        write_query,
        new_balance,
        account_id
    );
}
```

### Conditional Queries

```cpp
// CQL allows IF EXISTS and IF conditions
cassandra::Query conditional_update(
    "UPDATE users SET email = ? WHERE id = ? IF email = ?"
);

auto result = session->Execute(
    cassandra::Consistency::kQuorum,
    conditional_update,
    new_email,
    user_id,
    old_email
);

// Check if the condition was met
if (!result.Empty()) {
    bool success = result.AsSingleRow<bool>(cassandra::io::kFieldTag);
    if (success) {
        LOG_INFO("Email updated successfully");
    } else {
        LOG_INFO("Email was different, update skipped");
    }
}
```

### Efficient Bulk Operations

```cpp
// For inserting many rows efficiently
std::vector<UserData> users = GetUsersFromSource();

// Batch in chunks of reasonable size
const size_t batch_size = 100;
for (size_t i = 0; i < users.size(); i += batch_size) {
    cassandra::BatchQueryStore batch(cassandra::Consistency::kLocalQuorum);
    
    for (size_t j = i; j < i + batch_size && j < users.size(); ++j) {
        batch.AddQuery(
            "INSERT INTO users (id, name, email) VALUES (?, ?, ?)",
            users[j].id,
            users[j].name,
            users[j].email
        );
    }
    
    session->BatchExecute(batch);
    LOG_INFO("Inserted users {}-{}", i, i + batch_size);
}
```

### Query with Type Safety

```cpp
// Define a query as a constant to ensure it's always the same
constexpr cassandra::Query INSERT_USER_QUERY{
    "INSERT INTO users (id, name, email, created_at) VALUES (?, ?, ?, ?)"
};

struct User {
    int id;
    std::string name;
    std::string email;
    std::chrono::system_clock::time_point created_at;
};

auto InsertUser(cassandra::SessionPtr session, const User& user) {
    return session->Execute(
        cassandra::Consistency::kQuorum,
        INSERT_USER_QUERY,
        user.id,
        user.name,
        user.email,
        user.created_at
    );
}
```

## Best Practices

### 1. Reuse Session Objects

```cpp
// GOOD - Single session, reused across requests
class MyService {
    cassandra::SessionPtr _session;  // Shared across all handlers
};

// BAD - Creating new sessions repeatedly
void HandleRequest() {
    auto session = CreateNewSession();  // Don't do this!
    // Use session
}
```

### 2. Choose Appropriate Consistency Levels

```cpp
// For user profile reads where staleness is OK
session->Execute(cassandra::Consistency::kOne, read_query, user_id);

// For financial transactions
session->Execute(cassandra::Consistency::kQuorum, write_query, transaction_data);

// For critical operations
session->Execute(cassandra::Consistency::kAll, critical_query, data);
```

### 3. Use Prepared Queries for Repeated Operations

```cpp
// Define once as a constant
static const cassandra::Query GET_USER_QUERY{"SELECT * FROM users WHERE id = ?"};

// Reuse many times - query is automatically prepared and cached
auto result1 = session->Execute(cassandra::Consistency::kQuorum, GET_USER_QUERY, id1);
auto result2 = session->Execute(cassandra::Consistency::kQuorum, GET_USER_QUERY, id2);
auto result3 = session->Execute(cassandra::Consistency::kQuorum, GET_USER_QUERY, id3);
```

### 4. Batch Related Operations

```cpp
// Good - related operations in one batch
cassandra::BatchQueryStore batch(cassandra::Consistency::kQuorum);
batch.AddQuery("INSERT INTO user_events (user_id, event_type) VALUES (?, ?)", user_id, "login");
batch.AddQuery("UPDATE users SET last_login = ? WHERE id = ?", now, user_id);
session->BatchExecute(batch);

// Less efficient - separate calls
session->Execute(cassandra::Consistency::kQuorum, event_query, user_id, "login");
session->Execute(cassandra::Consistency::kQuorum, update_query, now, user_id);
```

### 5. Handle Errors Gracefully

```cpp
try {
    auto result = session->Execute(consistency_level, query, params...);
    return ProcessResult(result);
} catch (const cassandra::exceptions::ConnectionError& e) {
    LOG_ERROR("Temporary connection issue: {}", e.what());
    // Return error that client should retry
    throw std::runtime_error("Database temporarily unavailable");
} catch (const cassandra::exceptions::FrameError& e) {
    LOG_ERROR("Cassandra error: {}", e.what());
    // Likely a query error, not worth retrying
    throw std::runtime_error(std::string("Database error: ") + e.what());
}
```

### 6. Use Command Controls for Long Operations

```cpp
// Long-running query with extended timeout
cassandra::CommandControl long_timeout(
    std::chrono::milliseconds{60000},  // 60 second network timeout
    std::chrono::milliseconds{50000}   // 50 second statement timeout
);

auto result = session->Execute(
    cassandra::Consistency::kQuorum,
    long_timeout,
    expensive_query,
    params...
);
```

### 7. Consider Pagination for Large Result Sets

```cpp
// For large queries, fetch in chunks
// Cassandra supports LIMIT in queries
cassandra::Query paginated_query("SELECT * FROM logs WHERE user_id = ? LIMIT ?");

const int page_size = 100;
int offset = 0;

while (true) {
    auto result = session->Execute(
        cassandra::Consistency::kOne,
        paginated_query,
        user_id,
        page_size
    );
    
    if (result.Empty()) break;
    
    ProcessPage(result);
    offset += page_size;
}
```

## See Also

- @ref docs/tutorial/component.md - Setting up the Cassandra component
- @ref docs/tutorial/result_set.md - Processing query results
- @ref docs/tutorial/example_service.md - Complete service example