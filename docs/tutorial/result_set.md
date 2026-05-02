# Working with Cassandra ResultSet

The `ResultSet` class represents the results returned from a Cassandra query execution. It provides multiple ways to access and deserialize query results, from single rows to bulk extraction into containers.

## Overview

A `ResultSet` object:
- Contains results from a single query execution
- Provides metadata about the result (rows affected, columns affected)
- Allows extraction of results as typed C++ objects or structs
- Supports both single row and bulk container extraction
- Is thread-safe and backed by shared storage

ResultSet objects are returned by `Session::Execute()` and `Session::BatchExecute()` methods.

## Checking if Results are Empty

Always check if a result set contains rows before accessing data:

```cpp
#include <cassandra/session.hpp>
#include <cassandra/result_set.hpp>

cassandra::Query query("SELECT * FROM users WHERE id = ?");
auto result = session->Execute(cassandra::Consistency::kQuorum, query, user_id);

if (result.Empty()) {
    LOG_WARNING("No user found with id={}", user_id);
    return nullptr;
} else {
    // Safe to access result data
    auto user = result.AsSingleRow<User>(cassandra::io::kRowTag);
    return user;
}
```

The `Empty()` method returns true if:
- The result set contains no rows
- The underlying result wrapper is null

## Result Metadata

Query results provide metadata about what was returned:

```cpp
auto result = session->Execute(cassandra::Consistency::kQuorum, query, params...);

// For SELECT queries - number of rows returned
int row_count = result.RowsAffected();
LOG_DEBUG("Query returned {} rows", row_count);

// For INSERT/UPDATE/DELETE queries - number of rows modified
int modified_count = result.RowsAffected();
LOG_DEBUG("Modified {} rows", modified_count);

// Number of columns in each row
int column_count = result.ColumnsAffected();
LOG_DEBUG("Each row has {} columns", column_count);
```

## Extracting Single Rows

### Extract as Struct (Row-level)

For queries returning exactly one row, extract it as a typed struct:

```cpp
#include <cassandra/io/row_types.hpp>

// Define your data structure
struct User {
    cassandra::io::Int id;
    std::string name;
    std::string email;
};

// Execute query
cassandra::Query query("SELECT id, name, email FROM users WHERE id = ?");
auto result = session->Execute(cassandra::Consistency::kQuorum, query, user_id);

if (!result.Empty()) {
    // Extract entire row as struct
    User user = result.AsSingleRow<User>(cassandra::io::kRowTag);
    LOG_INFO("User: {} ({})", user.name, user.email);
}
```

### Extract as Single Value (Field-level)

For single-column results, extract just the value:

```cpp
// Query returns a single value
cassandra::Query count_query("SELECT COUNT(*) FROM users WHERE active = ?");
auto result = session->Execute(cassandra::Consistency::kOne, count_query, true);

if (!result.Empty()) {
    // Extract just the first column value
    int active_count = result.AsSingleRow<int>(cassandra::io::kFieldTag);
    LOG_INFO("Active users: {}", active_count);
}
```

### Direct Row Access

Access the first row directly for manual processing:

```cpp
auto result = session->Execute(cassandra::Consistency::kQuorum, query, id);

if (!result.Empty()) {
    cassandra::Row first_row = result.Front();
    
    // Process fields individually
    int user_id = first_row.As<int>(cassandra::io::kFieldTag);
    
    // Or extract as struct
    struct User { int id; std::string name; };
    User user = first_row.As<User>(cassandra::io::kRowTag);
}
```

## Extracting Multiple Rows

### Into a Container (Vector)

Extract all rows into a standard container:

```cpp
#include <cassandra/io/row_types.hpp>

struct Product {
    cassandra::io::Int id;
    std::string name;
    double price;
};

// Query returning multiple rows
cassandra::Query query("SELECT id, name, price FROM products WHERE category = ?");
auto result = session->Execute(cassandra::Consistency::kOne, query, category);

// Extract all rows into a vector
std::vector<Product> products = result.AsContainer<std::vector<Product>>(cassandra::io::kRowTag);

for (const auto& product : products) {
    LOG_DEBUG("Product: {} - ${}", product.name, product.price);
}
```

### Into Other Container Types

The `AsContainer` method works with any container supporting `push_back()` and `reserve()`:

```cpp
// Extract into a list
std::list<Product> product_list = result.AsContainer<std::list<Product>>(cassandra::io::kRowTag);

// Extract into a deque
std::deque<Product> product_deque = result.AsContainer<std::deque<Product>>(cassandra::io::kRowTag);

// Even custom containers that support the interface
MyCustomList<Product> products = result.AsContainer<MyCustomList<Product>>(cassandra::io::kRowTag);
```

## Working with Row Objects

The `Row` class provides low-level access to individual row data:

```cpp
cassandra::Row row = result.Front();

// Get single column value
int id = row.As<int>(cassandra::io::kFieldTag);

// Get entire row as struct
struct User { int id; std::string name; };
User user = row.As<User>(cassandra::io::kRowTag);

// Iterate through columns (if your Row type supports it)
// Most use cases are better served by struct extraction
```

## Type Conversions and Deserialization

### Supported Types

The driver automatically deserializes Cassandra types to C++ types:

```cpp
struct CompleteRow {
    // Integer types
    cassandra::io::Int int_val;
    cassandra::io::BigInt bigint_val;
    cassandra::io::SmallInt smallint_val;
    cassandra::io::TinyInt tinyint_val;
    
    // String types
    std::string text_val;
    std::string varchar_val;
    
    // Floating point
    float float_val;
    double double_val;
    
    // Boolean
    bool boolean_val;
    
    // Collections
    std::vector<std::string> list_val;
    std::set<int> set_val;
    std::map<std::string, int> map_val;
    
    // Optional fields (nulls)
    std::optional<std::string> optional_val;
};

auto result = session->Execute(consistency, query, params...);
auto row = result.AsSingleRow<CompleteRow>(cassandra::io::kRowTag);
```

### Type Tag Requirements

You must specify how to extract data using tags:

- `cassandra::io::kRowTag` - Extract entire row as a struct
- `cassandra::io::kFieldTag` - Extract single column value

```cpp
// CORRECT - Extract entire row
auto user = result.AsSingleRow<User>(cassandra::io::kRowTag);

// CORRECT - Extract single value
auto count = result.AsSingleRow<int>(cassandra::io::kFieldTag);

// INCORRECT - Missing tag (won't compile)
// auto user = result.AsSingleRow<User>();
```

### Handling Null Values

Cassandra supports NULL values, represented as `std::optional` in C++:

```cpp
struct UserProfile {
    int id;
    std::string name;
    std::optional<std::string> middle_name;  // Can be NULL
    std::optional<int> age;                   // Can be NULL
};

auto result = session->Execute(consistency, query, id);
if (!result.Empty()) {
    UserProfile profile = result.AsSingleRow<UserProfile>(cassandra::io::kRowTag);
    
    if (profile.middle_name.has_value()) {
        LOG_INFO("Middle name: {}", profile.middle_name.value());
    }
    
    if (profile.age) {
        LOG_INFO("Age: {}", *profile.age);
    }
}
```

## Common Patterns

### SELECT with Unknown Rows

```cpp
cassandra::Query query("SELECT id, name FROM users WHERE active = ?");
auto result = session->Execute(cassandra::Consistency::kOne, query, true);

struct User { int id; std::string name; };

if (!result.Empty()) {
    std::vector<User> users = result.AsContainer<std::vector<User>>(cassandra::io::kRowTag);
    LOG_INFO("Found {} active users", users.size());
}
```

### COUNT Query

```cpp
cassandra::Query count_query("SELECT COUNT(*) FROM users");
auto result = session->Execute(cassandra::Consistency::kOne, count_query);

if (!result.Empty()) {
    long count = result.AsSingleRow<cassandra::io::BigInt>(cassandra::io::kFieldTag);
    LOG_INFO("Total users: {}", count);
}
```

### INSERT/UPDATE/DELETE with Affected Count

```cpp
cassandra::Query delete_query("DELETE FROM users WHERE id = ?");
auto result = session->Execute(cassandra::Consistency::kQuorum, delete_query, user_id);

int deleted_count = result.RowsAffected();
if (deleted_count > 0) {
    LOG_INFO("Successfully deleted {} user(s)", deleted_count);
} else {
    LOG_WARNING("User not found");
}
```

### Conditional Updates

CQL supports IF clauses that return boolean results:

```cpp
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

if (!result.Empty()) {
    bool success = result.AsSingleRow<bool>(cassandra::io::kFieldTag);
    if (success) {
        LOG_INFO("Email updated successfully");
    } else {
        LOG_WARNING("Email mismatch - update not applied");
    }
}
```

## Error Handling

### Empty Result Handling

```cpp
auto result = session->Execute(consistency, query, params...);

// Always check before accessing
if (result.Empty()) {
    // Handle no results case
    return HandleNotFound();
}

// Safe to extract
return result.AsSingleRow<T>(cassandra::io::kRowTag);
```

### Type Mismatch Errors

```cpp
try {
    // If struct fields don't match columns, deserialization fails
    auto data = result.AsSingleRow<WrongStructType>(cassandra::io::kRowTag);
} catch (const cassandra::exceptions::Error& e) {
    LOG_ERROR("Deserialization failed: {}", e.what());
    // Handle type mismatch
}
```

### Container Extraction Errors

```cpp
try {
    auto rows = result.AsContainer<std::vector<MyStruct>>(cassandra::io::kRowTag);
} catch (const cassandra::exceptions::Error& e) {
    LOG_ERROR("Failed to extract rows: {}", e.what());
    // Handle error - may have extracted partial data
}
```

## Best Practices

### 1. Always Check Empty First

```cpp
// GOOD
auto result = session->Execute(consistency, query, params...);
if (!result.Empty()) {
    auto data = result.AsSingleRow<T>(cassandra::io::kRowTag);
    // Use data
}

// BAD - Undefined behavior if empty
auto result = session->Execute(consistency, query, params...);
auto data = result.AsSingleRow<T>(cassandra::io::kRowTag);  // Crash if empty!
```

### 2. Match Struct Fields to Query Columns

```cpp
// Good - fields match query columns in order
struct UserRow {
    int id;
    std::string name;
    std::string email;
};
// Query: SELECT id, name, email FROM users WHERE id = ?

// Bad - field mismatch will cause deserialization error
struct WrongRow {
    int id;
    std::string email;  // Wrong order!
    std::string name;
};
```

### 3. Use Optional for Nullable Columns

```cpp
// Good - handles NULL values
struct UserData {
    int id;
    std::string name;
    std::optional<std::string> phone;  // Can be NULL
};

// Bad - no NULL handling, will crash if column is NULL
struct BadData {
    int id;
    std::string name;
    std::string phone;  // Can't be NULL!
};
```

### 4. Validate Result Size

```cpp
auto result = session->Execute(consistency, query, params...);

// Check if result is within expected bounds
if (result.Empty()) {
    LOG_WARNING("No results");
    return {};
}

auto rows = result.AsContainer<std::vector<T>>(cassandra::io::kRowTag);
if (rows.size() > 1000) {
    LOG_ERROR("Unexpectedly large result set: {}", rows.size());
    // Implement pagination or filtering
}
```

### 5. Use Metadata for Validation

```cpp
auto result = session->Execute(consistency, insert_query, params...);

// Verify insert was successful
if (result.RowsAffected() == 0) {
    LOG_ERROR("Insert failed - 0 rows affected");
} else if (result.ColumnsAffected() > 0) {
    LOG_DEBUG("Insert returned {} columns", result.ColumnsAffected());
}
```

### 6. Define Reusable Row Structures

```cpp
// Define once and reuse throughout your codebase
namespace db_models {
    struct User {
        cassandra::io::Int id;
        std::string username;
        std::string email;
        std::optional<std::string> full_name;
        bool active;
    };
    
    struct Order {
        cassandra::io::BigInt order_id;
        int user_id;
        std::vector<std::string> items;
        double total;
    };
}

// Use in multiple places
auto user = result.AsSingleRow<db_models::User>(cassandra::io::kRowTag);
auto orders = result.AsContainer<std::vector<db_models::Order>>(cassandra::io::kRowTag);
```

### 7. Handle Large Result Sets

```cpp
// For queries that might return many rows, use pagination
cassandra::Query paginated_query("SELECT * FROM events WHERE user_id = ? LIMIT ?");

const int page_size = 100;
int processed = 0;

for (int offset = 0; ; offset += page_size) {
    auto result = session->Execute(
        cassandra::Consistency::kOne,
        paginated_query,
        user_id,
        page_size
    );
    
    if (result.Empty()) break;
    
    auto rows = result.AsContainer<std::vector<Event>>(cassandra::io::kRowTag);
    ProcessBatch(rows);
    processed += rows.size();
}

LOG_INFO("Processed {} total rows", processed);
```

## Examples

### Complete SELECT Example

```cpp
#include <cassandra/session.hpp>
#include <cassandra/result_set.hpp>

struct UserData {
    cassandra::io::Int id;
    std::string username;
    std::string email;
    bool active;
};

void HandleGetUser(cassandra::SessionPtr session, int user_id) {
    try {
        cassandra::Query query("SELECT id, username, email, active FROM users WHERE id = ?");
        auto result = session->Execute(cassandra::Consistency::kQuorum, query, user_id);
        
        if (result.Empty()) {
            LOG_WARNING("User {} not found", user_id);
            return;
        }
        
        UserData user = result.AsSingleRow<UserData>(cassandra::io::kRowTag);
        LOG_INFO("Found user: {} ({})", user.username, user.email);
        
        if (user.active) {
            LOG_DEBUG("User is active");
        } else {
            LOG_DEBUG("User is inactive");
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to retrieve user: {}", e.what());
    }
}
```

### Complete Bulk Extract Example

```cpp
#include <cassandra/session.hpp>
#include <cassandra/result_set.hpp>

struct OrderSummary {
    cassandra::io::BigInt order_id;
    std::string status;
    double total;
};

std::vector<OrderSummary> GetActiveOrders(cassandra::SessionPtr session) {
    cassandra::Query query("SELECT order_id, status, total FROM orders WHERE status = ?");
    auto result = session->Execute(cassandra::Consistency::kOne, query, "pending");
    
    if (result.Empty()) {
        LOG_INFO("No pending orders");
        return {};
    }
    
    std::vector<OrderSummary> orders = result.AsContainer<std::vector<OrderSummary>>(cassandra::io::kRowTag);
    LOG_INFO("Found {} pending orders", orders.size());
    return orders;
}
```

## See Also

- @ref docs/tutorial/session.md - Executing queries and getting ResultSet objects
- @ref docs/tutorial/component.md - Setting up the Cassandra component
- @ref docs/tutorial/example_service.md - Complete service example