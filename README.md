# userver-cql-driver

Асинхронный C++ драйвер для Apache Cassandra (Native Protocol V4), интегрированный с фреймворком [Userver](https://userver.tech/).

## Возможности

- Нативная интеграция с userver — компонент, secdist, task processor
- Выполнение запросов в стиле userver (без явных `future`/`.then()`)
- Пакетное выполнение запросов (`BatchExecute`)
- Компрессия тела запроса
- Кэширование prepared statements (NWayLRU)
- Пул соединений с настраиваемыми min/max и TTL
- Типобезопасная работа с результатами через агрегатные структуры — подход вдохновлён [uPg](https://userver.tech/db/db5/pg_process_results.html)
- Настройка уровней согласованности (consistency level) на уровне каждого запроса

## Документация

Полная документация, конфигурационный справочник и руководства — в [документации](docs/index.md).

| Раздел | Описание |
|--------|----------|
| [Компонент](docs/tutorial/component.md) | Регистрация, YAML-конфигурация, secdist |
| [Session](docs/tutorial/session.md) | Выполнение запросов, consistency levels, CommandControl |
| [ResultSet](docs/tutorial/result_set.md) | Извлечение строк и значений из результата |
| [Поддерживаемые типы](docs/tutorial/supported_data_types.md) | Маппинг CQL-типов на C++ |
| [Пример сервиса](docs/tutorial/example_service.md) | Полный рабочий пример |

## Быстрый старт

### 1. Регистрация компонента

```cpp
#include <cassandra/component.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::clients::dns::Component>()
        .Append<components::Cassandra>("cassandra-component")
        .Append<views::Cassandra>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
```

### 2. Конфигурация (`static_config.yaml`)

```yaml
cassandra-component:
  keyspace: my_keyspace
  blocking_task_processor: fs-task-processor
  dns_resolver: async
  min-pool-size: 5
  max-pool-size: 50
  persistent-prepared-statements: true
```

### 3. Адреса узлов (`secure_data.json`)

```json
{
    "cassandra_settings": {
        "keyspaces": {
            "my_keyspace": {
                "nodes": [
                    {
                        "contact-point": "127.0.0.1",
                        "port": 9042,
                        "allow-all": true,
                        "use-compression": false
                    }
                ]
            }
        }
    }
}
```

### 4. Использование в хендлере

```cpp
#include <cassandra/component.hpp>
#include <cassandra/query.hpp>
#include <cassandra/result_set.hpp>

namespace views {

const cassandra::Query kInsertQuery{
    "INSERT INTO my_ks.users (id, name) VALUES (?, ?)"
};
const cassandra::Query kSelectQuery{
    "SELECT id, name FROM my_ks.users WHERE id = ?"
};

struct UserRow {
    cassandra::io::Int id;
    std::string        name;
};

class MyHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "my-handler";

    MyHandler(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : HttpHandlerJsonBase(config, context),
        _session(context
                     .FindComponent<components::Cassandra>("cassandra-component")
                     .GetSessionPtr()) {}

    userver::formats::json::Value HandleRequestJsonThrow(
        const HttpRequest& request,
        const userver::formats::json::Value& body,
        RequestContext&
    ) const override {
        auto id   = body["id"].As<int>();
        auto name = body["name"].As<std::string>();

        // INSERT
        _session->Execute(cassandra::Consistency::kQuorum, kInsertQuery, id, name);

        // SELECT
        auto result = _session->Execute(
            cassandra::Consistency::kQuorum, kSelectQuery, id
        );

        if (result.Empty()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::NotFound);
            return {};
        }

        auto row = result.AsSingleRow<UserRow>(cassandra::io::kRowTag);
        return userver::formats::json::MakeObject("id", row.id, "name", row.name);
    }

private:
    cassandra::SessionPtr _session;
};

} // namespace views
```

## Лицензия

Распространяется под лицензией [Apache-2.0](http://www.apache.org/licenses/LICENSE-2.0).
