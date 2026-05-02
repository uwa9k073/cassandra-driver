# Supported Data Types

This page documents the C++ types that have actual IO serialization and
deserialization implementations in the driver. Only types listed here can be
used as query parameters or extracted from a `ResultSet`. Attempting to use
any other type will result in a compile-time error.

All IO specializations live under `cassandra/io/` and are pulled in
automatically when you include `<cassandra/session.hpp>`.

---

## Integer Types

**Header:** `cassandra/io/integral_types.hpp`

Defined as plain type aliases in `cassandra/io/cassandra_types.hpp`:

```cpp
namespace cassandra::io {
    using TinyInt  = std::int8_t;
    using SmallInt = std::int16_t;
    using Int      = std::int32_t;
    using BigInt   = std::int64_t;
    using Boolean  = bool;
}
```

| Cassandra CQL Type | C++ Type              | Width           |
|--------------------|-----------------------|-----------------|
| `TINYINT`          | `cassandra::io::TinyInt`  | 8-bit signed  |
| `SMALLINT`         | `cassandra::io::SmallInt` | 16-bit signed |
| `INT`              | `cassandra::io::Int`      | 32-bit signed |
| `BIGINT`           | `cassandra::io::BigInt`   | 64-bit signed |
| `BOOLEAN`          | `cassandra::io::Boolean`  | `bool`        |

Since all of these are plain `typedef`s over primitive types, you can pass
raw C++ literals and variables directly:

```cpp
#include <cassandra/session.hpp>

// Passing as query parameters
session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO metrics (sensor_id, value, active, flags) VALUES (?, ?, ?, ?)",
    42,                       // INT
    9'223'372'036LL,          // BIGINT
    true,                     // BOOLEAN
    static_cast<int8_t>(7)   // TINYINT
);

// In result structs the aliases document your intent clearly
struct MetricRow {
    cassandra::io::Int     sensor_id;
    cassandra::io::BigInt  value;
    cassandra::io::Boolean active;
    cassandra::io::TinyInt flags;
};

auto row = result.AsSingleRow<MetricRow>(cassandra::io::kRowTag);
// Arithmetic works directly on the underlying primitive
int next_id = row.sensor_id + 1;
```

### Internal protocol types

`cassandra::io::Byte` (`uint8_t`) and `cassandra::io::Short` (`uint16_t`)
also have IO implementations but they exist exclusively for the Cassandra
Native Protocol frame format. Do not use them for CQL column values.

---

## Floating-Point Types

**Header:** `cassandra/io/floating_point_types.hpp`

```cpp
namespace cassandra::io {
    using Float  = float;
    using Double = double;
}
```

| Cassandra CQL Type | C++ Type               |
|--------------------|------------------------|
| `FLOAT`            | `cassandra::io::Float` |
| `DOUBLE`           | `cassandra::io::Double`|

Values are serialized in IEEE 754 big-endian format, matching the Cassandra
wire protocol.

```cpp
struct SensorReading {
    cassandra::io::Int    id;
    cassandra::io::Float  temperature;   // FLOAT
    cassandra::io::Double pressure;      // DOUBLE
};

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO readings (id, temperature, pressure) VALUES (?, ?, ?)",
    sensor_id,
    23.5f,    // FLOAT
    1013.25   // DOUBLE
);

auto row = result.AsSingleRow<SensorReading>(cassandra::io::kRowTag);
```

---

## String Types

**Header:** `cassandra/io/string_types.hpp`

The driver provides three string types with IO support, differing only in
how the byte length is encoded on the wire:

```cpp
namespace cassandra::io {
    // Length encoded as uint16_t (2 bytes)
    using String     = userver::utils::StrongTypedef<Short, std::string>;

    // Length encoded as int32_t (4 bytes)
    using LongString = userver::utils::StrongTypedef<Int,   std::string>;
}
```

| Use case                       | C++ Type                    | Length prefix |
|--------------------------------|-----------------------------|---------------|
| CQL `TEXT` / `VARCHAR` columns | `std::string`               | `uint16_t`    |
| Protocol short string fields   | `cassandra::io::String`     | `uint16_t`    |
| Protocol long string fields    | `cassandra::io::LongString` | `int32_t`     |

For mapping CQL `TEXT` or `VARCHAR` columns, **use `std::string`**. It has
its own `Input`/`Output` specializations in the driver:

```cpp
struct UserRow {
    cassandra::io::Int id;
    std::string        username;   // TEXT or VARCHAR
    std::string        email;      // TEXT or VARCHAR
};

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO users (id, username, email) VALUES (?, ?, ?)",
    user_id,
    std::string{"alice"},
    std::string{"alice@example.com"}
);

auto row = result.AsSingleRow<UserRow>(cassandra::io::kRowTag);
```

`cassandra::io::String` and `cassandra::io::LongString` are `StrongTypedef`
wrappers. Access the underlying `std::string` with `.GetUnderlying()`.
They are used internally for protocol metadata (e.g., option maps,
error messages) and are not needed for ordinary CQL column interaction.

---

## Binary / Bytes Types

**Header:** `cassandra/io/list_types.hpp`

```cpp
namespace cassandra::io {
    // Length encoded as int32_t (4 bytes) — matches CQL [bytes]
    using Bytes      = userver::utils::StrongTypedef<Int,   std::vector<std::byte>>;

    // Length encoded as uint16_t (2 bytes) — matches CQL [short bytes]
    using ShortBytes = userver::utils::StrongTypedef<Short, std::vector<std::byte>>;
}
```

| Cassandra CQL type | C++ Type                  | Length prefix |
|--------------------|---------------------------|---------------|
| `BLOB`             | `cassandra::io::Bytes`    | `int32_t`     |
| Protocol short bytes | `cassandra::io::ShortBytes` | `uint16_t` |

For CQL `BLOB` columns use `cassandra::io::Bytes`. Its wire format
(`[int32 length][raw bytes]`) matches the CQL `[bytes]` protocol encoding
exactly. `ShortBytes` is for internal protocol fields that use a 2-byte
length; do not use it for CQL column values.

```cpp
#include <cassandra/session.hpp>
#include <vector>
#include <cstddef>

// Build binary payload
std::vector<std::byte> payload = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}};
cassandra::io::Bytes blob_value{payload};

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO files (id, data) VALUES (?, ?)",
    file_id,
    blob_value
);

// Read back
struct FileRow {
    cassandra::io::Int   id;
    cassandra::io::Bytes data;   // BLOB
};

auto row = result.AsSingleRow<FileRow>(cassandra::io::kRowTag);
const auto& raw = row.data.GetUnderlying();  // std::vector<std::byte>
```

> **Note:** `cassandra::io::Blob` (`std::vector<std::byte>`) is declared in
> `cassandra_types.hpp` as a convenience alias but does **not** have IO
> specializations. Always use `cassandra::io::Bytes` for CQL `BLOB` columns.

---

## Sequence / List Types

**Header:** `cassandra/io/list_types.hpp`

The driver provides a generic IO implementation for any C++ type that
satisfies the `SequenceContainerConcept`:

```cpp
template <typename T>
concept SequenceContainerConcept = requires(T container) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;
    { container.begin()  } -> std::same_as<typename T::iterator>;
    { container.end()    } -> std::same_as<typename T::iterator>;
    { container.cbegin() } -> std::same_as<typename T::const_iterator>;
    { container.cend()   } -> std::same_as<typename T::const_iterator>;
    { container.size()   } -> std::convertible_to<typename T::size_type>;
    { container.empty()  } -> std::convertible_to<bool>;
    { container.front()  } -> std::same_as<typename T::value_type&>;
    { container.back()   } -> std::same_as<typename T::value_type&>;
};
```

Standard containers that satisfy this concept:

| Container           | Satisfies concept? | Notes                          |
|---------------------|--------------------|--------------------------------|
| `std::vector<T>`    | ✅ Yes              | Recommended for LIST and SET   |
| `std::list<T>`      | ✅ Yes              | Doubly-linked list             |
| `std::deque<T>`     | ✅ Yes              | Double-ended queue             |
| `std::set<T>`       | ❌ No               | Missing `front()` / `back()`   |
| `std::unordered_set<T>` | ❌ No           | Missing `front()` / `back()`   |

The element type `T` must itself have IO support.

### CQL LIST

```cpp
// Write a LIST<TEXT>
std::vector<std::string> tags = {"c++", "userver", "cassandra"};

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO posts (id, tags) VALUES (?, ?)",
    post_id,
    tags
);

// Read back
struct PostRow {
    cassandra::io::Int           id;
    std::vector<std::string>     tags;   // LIST<TEXT>
};

auto row = result.AsSingleRow<PostRow>(cassandra::io::kRowTag);
for (const auto& tag : row.tags) {
    LOG_DEBUG("tag: {}", tag);
}
```

### CQL SET

Cassandra `SET` uses the same binary wire format as `LIST` (count + elements).
The driver does not enforce uniqueness on the C++ side; use `std::vector<T>`
to read and write SET columns:

```cpp
// Write a SET<TEXT> using std::vector — Cassandra deduplicates on the server
std::vector<std::string> roles = {"admin", "editor", "admin"};   // duplicate removed by Cassandra

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO user_roles (user_id, roles) VALUES (?, ?)",
    user_id,
    roles
);

// Read back
struct UserRoleRow {
    cassandra::io::Int       user_id;
    std::vector<std::string> roles;   // SET<TEXT>
};
```

### Built-in sequence type alias

`cassandra::io::StringList` (`std::vector<cassandra::io::String>`) has an
explicit specialization that uses a `uint16_t` element count instead of the
default `int32_t`. It is used internally in the Cassandra protocol for
metadata; you do not need it for ordinary CQL column values.

---

## Map Types

**Header:** `cassandra/io/map_types.hpp`

The driver provides a generic IO implementation for any C++ type that
satisfies the `MapConcept`:

```cpp
template <typename T>
concept MapConcept = requires(T container) {
    typename T::value_type;
    typename T::key_type;
    typename T::mapped_type;
    // begin, end, cbegin, cend, size, empty, emplace (returning iterator
    // or pair<iterator,bool>)
};
```

Standard containers that satisfy this concept:

| Container                         | Notes                     |
|-----------------------------------|---------------------------|
| `std::map<K, V>`                  | Ordered map               |
| `std::unordered_map<K, V>`        | Unordered map             |
| `std::multimap<K, V>`             | Ordered, allows duplicates |

Both key type `K` and value type `V` must have IO support.

### CQL MAP

```cpp
// Write a MAP<TEXT, TEXT>
std::map<std::string, std::string> metadata = {
    {"region", "us-east-1"},
    {"tier",   "premium"}
};

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO tenants (id, metadata) VALUES (?, ?)",
    tenant_id,
    metadata
);

// Read back
struct TenantRow {
    cassandra::io::Int                    id;
    std::map<std::string, std::string>    metadata;   // MAP<TEXT, TEXT>
};

auto row = result.AsSingleRow<TenantRow>(cassandra::io::kRowTag);
```

```cpp
// MAP<TEXT, INT>
std::unordered_map<std::string, cassandra::io::Int> scores = {
    {"level_1", 840},
    {"level_2", 1200}
};

session->Execute(
    cassandra::Consistency::kQuorum,
    "INSERT INTO player_scores (id, scores) VALUES (?, ?)",
    player_id,
    scores
);
```

### Built-in map type aliases

These aliases have explicit specializations using a `uint16_t` count prefix
(matching the Cassandra protocol metadata encoding) and are used internally
by the driver. You do not need them for CQL MAP columns.

| Alias                         | Underlying C++ type                                        |
|-------------------------------|-------------------------------------------------------------|
| `cassandra::io::StringMap`    | `std::unordered_map<String, String>`                        |
| `cassandra::io::StringMultiMap` | `std::unordered_map<String, StringList>`                  |
| `cassandra::io::BytesMap`     | `std::unordered_map<String, Bytes>`                         |

---

## Row / Composite Types

**Header:** `cassandra/io/row_types.hpp`

Row-level deserialization turns a full result row into a C++ value. Two
categories are supported:

### 1. Aggregate structs (via `boost::pfr` reflection)

Any plain aggregate struct that:
- is not in the `std::` or `boost::` namespace,
- is not polymorphic or a union,
- has at least one field,

is automatically reflectable as a row type. No macros or extra code are
required.

```cpp
// Fields are matched positionally to SELECT columns
struct OrderRow {
    cassandra::io::BigInt  order_id;
    cassandra::io::Int     user_id;
    std::string            status;
    cassandra::io::Double  total;
};

cassandra::Query q(
    "SELECT order_id, user_id, status, total FROM orders WHERE order_id = ?"
);
auto result = session->Execute(cassandra::Consistency::kOne, q, order_id);
auto row = result.AsSingleRow<OrderRow>(cassandra::io::kRowTag);
```

### 2. `std::tuple<T...>`

```cpp
using OrderTuple = std::tuple<cassandra::io::BigInt, cassandra::io::Int,
                              std::string, cassandra::io::Double>;

auto row = result.AsSingleRow<OrderTuple>(cassandra::io::kRowTag);
auto order_id = std::get<0>(row);
auto user_id  = std::get<1>(row);
```

### Extraction tags

| Tag                        | When to use                                       |
|----------------------------|---------------------------------------------------|
| `cassandra::io::kRowTag`   | Deserialize all columns into a struct or tuple    |
| `cassandra::io::kFieldTag` | Extract the first column as a single scalar value |

```cpp
// Single scalar from the first column
cassandra::Query count_q("SELECT COUNT(*) FROM users WHERE active = ?");
auto result = session->Execute(cassandra::Consistency::kOne, count_q, true);
cassandra::io::BigInt count = result.AsSingleRow<cassandra::io::BigInt>(cassandra::io::kFieldTag);

// Full row into struct
cassandra::Query user_q("SELECT id, username, email FROM users WHERE id = ?");
result = session->Execute(cassandra::Consistency::kOne, user_q, user_id);
struct UserRow { cassandra::io::Int id; std::string username; std::string email; };
UserRow user = result.AsSingleRow<UserRow>(cassandra::io::kRowTag);
```

---

## Complete Reference Table

| Cassandra CQL Type | C++ Type                                 | Header                              |
|--------------------|------------------------------------------|-------------------------------------|
| `BOOLEAN`          | `cassandra::io::Boolean` (`bool`)        | `cassandra/io/integral_types.hpp`   |
| `TINYINT`          | `cassandra::io::TinyInt` (`int8_t`)      | `cassandra/io/integral_types.hpp`   |
| `SMALLINT`         | `cassandra::io::SmallInt` (`int16_t`)    | `cassandra/io/integral_types.hpp`   |
| `INT`              | `cassandra::io::Int` (`int32_t`)         | `cassandra/io/integral_types.hpp`   |
| `BIGINT`           | `cassandra::io::BigInt` (`int64_t`)      | `cassandra/io/integral_types.hpp`   |
| `FLOAT`            | `cassandra::io::Float` (`float`)         | `cassandra/io/floating_point_types.hpp` |
| `DOUBLE`           | `cassandra::io::Double` (`double`)       | `cassandra/io/floating_point_types.hpp` |
| `TEXT` / `VARCHAR` | `std::string`                            | `cassandra/io/string_types.hpp`     |
| `BLOB`             | `cassandra::io::Bytes`                   | `cassandra/io/list_types.hpp`       |
| `LIST<T>`          | `std::vector<T>`, `std::list<T>`, `std::deque<T>` | `cassandra/io/list_types.hpp` |
| `SET<T>`           | `std::vector<T>` (see note above)        | `cassandra/io/list_types.hpp`       |
| `MAP<K, V>`        | `std::map<K,V>`, `std::unordered_map<K,V>` | `cassandra/io/map_types.hpp`      |
| Row (struct)       | Any non-std/non-boost aggregate struct   | `cassandra/io/row_types.hpp`        |
| Row (tuple)        | `std::tuple<T...>`                       | `cassandra/io/row_types.hpp`        |
| Nullable column    | `std::optional<T>`                       | *(wraps any supported type)*        |

### Types with no IO implementation

The following types appear in the driver's type declarations but do **not**
have `BufferParser` / `BufferFormatter` specializations and **cannot** be
used for CQL column serialization:

| Cassandra CQL Type | Status                                      |
|--------------------|---------------------------------------------|
| `UUID` / `TIMEUUID`| Not implemented                             |
| `TIMESTAMP`        | Not implemented                             |
| `DATE`             | Declared as alias, no IO specialization     |
| `TIME`             | Not implemented                             |
| `DURATION`         | Not implemented                             |
| `VARINT`           | Not implemented                             |
| `DECIMAL`          | Not implemented                             |
| `COUNTER`          | Not implemented                             |
| `INET`             | Declared as alias, no IO specialization     |

---

## Common Mistakes

### Struct field order must match SELECT column order

Fields are deserialized **positionally**: the first struct field receives the
first column, the second field receives the second column, and so on.

```cpp
// Query: SELECT id, name, score FROM players
struct PlayerRow {
    cassandra::io::Int    id;      // column 0
    std::string           name;    // column 1
    cassandra::io::Double score;   // column 2
};

// WRONG — mismatched order corrupts data or causes a runtime error
struct BadRow {
    std::string           name;    // reads id (int) as string — error!
    cassandra::io::Int    id;
    cassandra::io::Double score;
};
```

### `std::set<T>` is not supported

`std::set<T>` does not satisfy `SequenceContainerConcept` because it lacks
`front()` and `back()`. Use `std::vector<T>` for CQL `SET` columns instead:

```cpp
// WRONG — does not compile
std::set<std::string> roles;
// ...
struct Row { std::set<std::string> roles; };   // compile error

// CORRECT
std::vector<std::string> roles;
struct Row { std::vector<std::string> roles; };
```

### `cassandra::io::Blob` has no IO

The alias `cassandra::io::Blob` (`std::vector<std::byte>`) is declared for
convenience but has no IO specialization. Always use `cassandra::io::Bytes`
for CQL `BLOB` columns:

```cpp
// WRONG — compile error at IO instantiation
cassandra::io::Blob bad_blob = ...;
session->Execute(query, bad_blob);

// CORRECT
cassandra::io::Bytes good_blob{payload_vector};
session->Execute(query, good_blob);
```

---

## See Also

- @ref docs/tutorial/result_set.md — Extracting typed results from a `ResultSet`
- @ref docs/tutorial/session.md — Executing queries with typed parameters
- @ref docs/tutorial/example_service.md — Complete service showing real-world type usage