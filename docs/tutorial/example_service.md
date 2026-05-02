# Writing an Example Service with Cassandra

This tutorial walks you through building a complete microservice using the userver framework and Cassandra driver. We'll create a user management service that demonstrates querying, inserting, updating, batch operations, and error handling.

## Service Overview

We'll build a simple user management service with HTTP endpoints:
- `POST /users` - Create a new user
- `GET /users/{id}` - Get user details
- `PUT /users/{id}` - Update user information
- `DELETE /users/{id}` - Delete a user
- `POST /users/batch` - Batch insert multiple users

## Project Structure

```
example_service/
├── CMakeLists.txt
├── main.cpp
├── configs/
│   ├── static_config.yaml
│   ├── config_vars.yaml
│   ├── config_vars.testing.yaml
│   └── secure_data.json
└── README.md
```

## Step 1: Project Configuration

### CMakeLists.txt

```cmake
add_executable(${CASSANDRA_ROOT_PROJECT_NAME}_example-service main.cpp)

target_link_libraries(
    ${CASSANDRA_ROOT_PROJECT_NAME}_example-service
    PRIVATE userver::core 
    PRIVATE userver::cassandra
)

target_include_directories(${CASSANDRA_ROOT_PROJECT_NAME}_example-service PRIVATE ../../include)
```

### static_config.yaml

```yaml
components_manager:
  event_thread_pool:
    threads: $event-threads
    threads#fallback: 2

  task_processors:
    main-task-processor:
      worker_threads: $worker-threads
      worker_threads#fallback: 4

    fs-task-processor:
      worker_threads: $worker-fs-threads
      worker_threads#fallback: 2

  default_task_processor: main-task-processor

  components:
    server:
      listener:
        port: $server-port
        port#fallback: 8080
        task_processor: main-task-processor

    logging:
      fs-task-processor: fs-task-processor
      loggers:
        default:
          file_path: "@stderr"
          level: $logger-level
          level#fallback: info
          overflow_behavior: discard

    dynamic-config: {}

    testsuite-support: {}

    secdist: {}
    default-secdist-provider:
      config: config/secure_data.json
      config#env: SECDIST_PATH

    dns-client:
      fs-task-processor: fs-task-processor

    http-client:
      fs-task-processor: fs-task-processor

    handler-ping:
      path: /ping
      method: GET
      task_processor: main-task-processor
      throttling_enabled: false

    # Cassandra component configuration
    cassandra-component:
      keyspace: $cassandra-keyspace
      blocking_task_processor: fs-task-processor
      dns_resolver: async
      min-pool-size: 5
      max-pool-size: 50
      sync-start: false
      persistent-prepared-statements: true
      max-ttl-sec: 3600
      prepared-statement-cache-ways: 16
      prepared-statement-cache-way-size: 200
      max-queue-size: 1000

    # User service handlers
    user-create-handler:
      path: /users
      method: POST
      task_processor: main-task-processor

    user-get-handler:
      path: /users/{id}
      method: GET
      task_processor: main-task-processor

    user-update-handler:
      path: /users/{id}
      method: PUT
      task_processor: main-task-processor

    user-delete-handler:
      path: /users/{id}
      method: DELETE
      task_processor: main-task-processor

    user-batch-handler:
      path: /users/batch
      method: POST
      task_processor: main-task-processor
```

### config_vars.yaml

```yaml
cassandra-keyspace: users_service
server-port: 8080
logger-level: info
worker-threads: 4
worker-fs-threads: 2
event-threads: 2
```

### secure_data.json

```json
{
    "cassandra_settings": {
        "keyspaces": {
            "users_service": {
                "nodes": [
                    {
                        "use-ssl": false,
                        "use-compression": false,
                        "allow-all": true,
                        "contact-point": "localhost",
                        "port": 9042
                    }
                ]
            }
        }
    }
}
```

## Step 2: Create Cassandra Schema

Before running the service, create the keyspace and table:

```sql
CREATE KEYSPACE IF NOT EXISTS users_service 
WITH REPLICATION = {'class': 'SimpleStrategy', 'replication_factor': 1};

USE users_service;

CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY,
    username TEXT,
    email TEXT,
    full_name TEXT,
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    is_active BOOLEAN
);

CREATE INDEX IF NOT EXISTS ON users (email);
```

## Step 3: Define C++ Data Models

```cpp
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <chrono>
#include <optional>
#include <string>

struct User {
    cassandra::io::Uuid id;
    std::string username;
    std::string email;
    std::optional<std::string> full_name;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    bool is_active;
};

// JSON serialization for API responses
userver::formats::json::Value 
Serialize(const User& user, userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = user.id;
    builder["username"] = user.username;
    builder["email"] = user.email;
    builder["full_name"] = user.full_name;
    builder["created_at"] = user.created_at;
    builder["updated_at"] = user.updated_at;
    builder["is_active"] = user.is_active;
    return builder.ExtractValue();
}

// JSON deserialization for API requests
struct CreateUserRequest {
    std::string username;
    std::string email;
    std::optional<std::string> full_name;
};

CreateUserRequest 
Parse(const userver::formats::json::Value& value, 
      userver::formats::parse::To<CreateUserRequest>) {
    return {
        value["username"].As<std::string>(),
        value["email"].As<std::string>(),
        value["full_name"].As<std::optional<std::string>>()
    };
}
```

## Step 4: Define Query Constants

Define all queries as static constants for consistency and reusability:

```cpp
namespace queries {
    const cassandra::Query CREATE_USER{
        "INSERT INTO users (id, username, email, full_name, created_at, updated_at, is_active) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
    };

    const cassandra::Query GET_USER{
        "SELECT id, username, email, full_name, created_at, updated_at, is_active "
        "FROM users WHERE id = ?"
    };

    const cassandra::Query UPDATE_USER{
        "UPDATE users SET username = ?, email = ?, full_name = ?, updated_at = ?, is_active = ? "
        "WHERE id = ?"
    };

    const cassandra::Query DELETE_USER{
        "DELETE FROM users WHERE id = ?"
    };

    const cassandra::Query GET_USER_BY_EMAIL{
        "SELECT id, username, email, full_name, created_at, updated_at, is_active "
        "FROM users WHERE email = ? ALLOW FILTERING"
    };
}
```

## Step 5: Implement HTTP Handlers

### Create User Handler

```cpp
class CreateUserHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "user-create-handler";

    CreateUserHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : HttpHandlerJsonBase(config, context),
          _session(context.FindComponent<components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const userver::formats::json::Value& request_json,
        RequestContext& /*context*/
    ) const override {
        try {
            auto req = request_json.As<CreateUserRequest>();

            // Generate new UUID for user
            cassandra::io::Uuid user_id = cassandra::io::Uuid::NewRandom();
            auto now = std::chrono::system_clock::now();

            // Insert user
            _session->Execute(
                cassandra::Consistency::kQuorum,
                queries::CREATE_USER,
                user_id,
                req.username,
                req.email,
                req.full_name,
                now,
                now,
                true
            );

            LOG_INFO("Created user: {} ({})", req.username, user_id);

            // Return created user
            User user{user_id, req.username, req.email, req.full_name, now, now, true};
            return userver::formats::json::ValueBuilder(user).ExtractValue();

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create user: {}", e.what());
            request.SetResponseStatus(userver::server::http::HttpStatus::BadRequest);
            return userver::formats::json::MakeObject("error", e.what());
        }
    }

private:
    cassandra::SessionPtr _session;
};
```

### Get User Handler

```cpp
class GetUserHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "user-get-handler";

    GetUserHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : HttpHandlerJsonBase(config, context),
          _session(context.FindComponent<components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const userver::formats::json::Value& /*request_json*/,
        RequestContext& /*context*/
    ) const override {
        try {
            auto user_id_str = request.GetPathArg("id");
            cassandra::io::Uuid user_id = cassandra::io::Uuid::FromString(user_id_str);

            // Query user
            auto result = _session->Execute(
                cassandra::Consistency::kQuorum,
                queries::GET_USER,
                user_id
            );

            if (result.Empty()) {
                LOG_WARNING("User not found: {}", user_id);
                request.SetResponseStatus(userver::server::http::HttpStatus::NotFound);
                return userver::formats::json::MakeObject("error", "User not found");
            }

            // Extract and return user
            User user = result.AsSingleRow<User>(cassandra::io::kRowTag);
            return userver::formats::json::ValueBuilder(user).ExtractValue();

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to get user: {}", e.what());
            request.SetResponseStatus(userver::server::http::HttpStatus::BadRequest);
            return userver::formats::json::MakeObject("error", e.what());
        }
    }

private:
    cassandra::SessionPtr _session;
};
```

### Update User Handler

```cpp
class UpdateUserHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "user-update-handler";

    UpdateUserHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : HttpHandlerJsonBase(config, context),
          _session(context.FindComponent<components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const userver::formats::json::Value& request_json,
        RequestContext& /*context*/
    ) const override {
        try {
            auto user_id_str = request.GetPathArg("id");
            cassandra::io::Uuid user_id = cassandra::io::Uuid::FromString(user_id_str);

            auto req = request_json.As<CreateUserRequest>();
            auto now = std::chrono::system_clock::now();

            // Update user
            _session->Execute(
                cassandra::Consistency::kQuorum,
                queries::UPDATE_USER,
                req.username,
                req.email,
                req.full_name,
                now,
                true,
                user_id
            );

            LOG_INFO("Updated user: {}", user_id);

            // Get and return updated user
            auto result = _session->Execute(
                cassandra::Consistency::kQuorum,
                queries::GET_USER,
                user_id
            );

            if (result.Empty()) {
                request.SetResponseStatus(userver::server::http::HttpStatus::NotFound);
                return userver::formats::json::MakeObject("error", "User not found");
            }

            User user = result.AsSingleRow<User>(cassandra::io::kRowTag);
            return userver::formats::json::ValueBuilder(user).ExtractValue();

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to update user: {}", e.what());
            request.SetResponseStatus(userver::server::http::HttpStatus::BadRequest);
            return userver::formats::json::MakeObject("error", e.what());
        }
    }

private:
    cassandra::SessionPtr _session;
};
```

### Delete User Handler

```cpp
class DeleteUserHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "user-delete-handler";

    DeleteUserHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : HttpHandlerJsonBase(config, context),
          _session(context.FindComponent<components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const userver::formats::json::Value& /*request_json*/,
        RequestContext& /*context*/
    ) const override {
        try {
            auto user_id_str = request.GetPathArg("id");
            cassandra::io::Uuid user_id = cassandra::io::Uuid::FromString(user_id_str);

            _session->Execute(cassandra::Consistency::kQuorum, queries::DELETE_USER, user_id);

            LOG_INFO("Deleted user: {}", user_id);

            return userver::formats::json::MakeObject("success", true);

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to delete user: {}", e.what());
            request.SetResponseStatus(userver::server::http::HttpStatus::BadRequest);
            return userver::formats::json::MakeObject("error", e.what());
        }
    }

private:
    cassandra::SessionPtr _session;
};
```

### Batch User Handler

```cpp
class BatchUserHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "user-batch-handler";

    BatchUserHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
        : HttpHandlerJsonBase(config, context),
          _session(context.FindComponent<components::Cassandra>("cassandra-component")
                       .GetSessionPtr()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const userver::formats::json::Value& request_json,
        RequestContext& /*context*/
    ) const override {
        try {
            auto users_data = request_json["users"].As<std::vector<CreateUserRequest>>();

            cassandra::BatchQueryStore batch(cassandra::Consistency::kQuorum);
            auto now = std::chrono::system_clock::now();
            int count = 0;

            for (const auto& user_req : users_data) {
                cassandra::io::Uuid user_id = cassandra::io::Uuid::NewRandom();
                batch.AddQuery(
                    queries::CREATE_USER,
                    user_id,
                    user_req.username,
                    user_req.email,
                    user_req.full_name,
                    now,
                    now,
                    true
                );
                count++;
            }

            _session->BatchExecute(batch);

            LOG_INFO("Batch inserted {} users", count);

            return userver::formats::json::MakeObject("inserted", count);

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to batch insert users: {}", e.what());
            request.SetResponseStatus(userver::server::http::HttpStatus::BadRequest);
            return userver::formats::json::MakeObject("error", e.what());
        }
    }

private:
    cassandra::SessionPtr _session;
};
```

## Step 6: Main Entry Point

```cpp
#include <userver/clients/dns/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <cassandra/component.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
                              .Append<userver::server::handlers::Ping>()
                              .Append<userver::components::TestsuiteSupport>()
                              .Append<userver::components::Secdist>()
                              .Append<userver::components::DefaultSecdistProvider>()
                              .Append<userver::clients::dns::Component>()
                              .Append<components::Cassandra>("cassandra-component")
                              .Append<handlers::CreateUserHandler>()
                              .Append<handlers::GetUserHandler>()
                              .Append<handlers::UpdateUserHandler>()
                              .Append<handlers::DeleteUserHandler>()
                              .Append<handlers::BatchUserHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
```

## Building and Running

### Build the Service

```bash
mkdir build
cd build
cmake ..
make
```

### Run the Service

```bash
# Start Cassandra (if using Docker)
docker run -d -p 9042:9042 cassandra:latest

# Wait for Cassandra to start
sleep 10

# Create keyspace and table
cqlsh localhost 9042 < schema.sql

# Run the service
./example_service --config config/static_config.yaml
```

## Testing the API

### Create a User

```bash
curl -X POST http://localhost:8080/users \
  -H "Content-Type: application/json" \
  -d '{
    "username": "alice",
    "email": "alice@example.com",
    "full_name": "Alice Smith"
  }'
```

### Get a User

```bash
curl http://localhost:8080/users/{user-id}
```

### Update a User

```bash
curl -X PUT http://localhost:8080/users/{user-id} \
  -H "Content-Type: application/json" \
  -d '{
    "username": "alice_updated",
    "email": "alice.new@example.com",
    "full_name": "Alice Smith Updated"
  }'
```

### Batch Insert Users

```bash
curl -X POST http://localhost:8080/users/batch \
  -H "Content-Type: application/json" \
  -d '{
    "users": [
      {"username": "bob", "email": "bob@example.com"},
      {"username": "charlie", "email": "charlie@example.com"},
      {"username": "diana", "email": "diana@example.com"}
    ]
  }'
```

### Delete a User

```bash
curl -X DELETE http://localhost:8080/users/{user-id}
```

## Key Concepts Demonstrated

### 1. Component Integration
The service shows how to integrate the Cassandra component with other userver components and retrieve the session from handlers through dependency injection.

### 2. Query Definition
Queries are defined as static constants for consistency, type safety, and easier maintenance. This pattern allows queries to be prepared once and reused.

### 3. Data Models
C++ structs represent database entities with JSON serialization support for API communication.

### 4. Single Query Execution
Standard query execution with parameters, consistency levels, and result extraction.

### 5. Batch Operations
Multiple operations grouped together for efficient transmission and execution on the server.

### 6. Error Handling
Comprehensive error handling with appropriate HTTP status codes and detailed logging.

### 7. Configuration
Complete service configuration using YAML for static settings and JSON for sensitive data like database credentials.

## Performance Considerations

### Connection Pooling
The component automatically manages connection pools with configurable min/max sizes. Adjust these based on your expected concurrency.

### Prepared Statements
With `persistent-prepared-statements: true`, repeated queries are automatically cached on the server, improving performance.

### Consistency Levels
Use `kLocalOne` or `kOne` for fast operations where some staleness is acceptable. Use `kQuorum` or `kAll` for critical operations requiring strong consistency.

### Batch Operations
Group related mutations together to reduce network round trips and improve throughput.

## Next Steps

### Advanced Features
- Add request validation and sanitization
- Implement pagination for large result sets
- Add authentication and authorization
- Implement rate limiting
- Add distributed tracing
- Add circuit breakers for resilience

### Testing
- Add unit tests for handlers
- Add integration tests with Cassandra
- Add load testing with multiple concurrent users

## See Also

- @ref docs/tutorial/component.md - Detailed component configuration guide
- @ref docs/tutorial/session.md - Session query execution reference
- @ref docs/tutorial/result_set.md - Result set processing reference