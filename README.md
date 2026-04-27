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

Полная документация, конфигурационный справочник и руководства — в [`docs/index.md`](docs/index.md).

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

---

## Сравнение с cpv-project/cpv-cql-driver

[cpv-cql-driver](https://github.com/cpv-project/cpv-cql-driver) — ещё один C++ драйвер для Cassandra, построенный на фреймворке [Seastar](https://seastar.io/).
Ниже приведено сравнение подходов на одинаковых задачах.

### Модель асинхронности

cpv использует фьючерсы Seastar с цепочками `.then()`. Этот драйвер использует корутины userver — код выглядит синхронно, но выполняется асинхронно в рамках планировщика userver.

**cpv-cql-driver**
```cpp
session.query(cql::Command("SELECT id, name FROM users WHERE id = ?")
    .addParameter(cql::Int(42))
    .setConsistency(cql::ConsistencyLevel::Quorum))
.then([] (cql::ResultSet result) {
    cql::Int  id;
    cql::Text name;
    result.fill(id, name);
    std::cout << id << " " << name << "\n";
});
```

**userver-cql-driver**
```cpp
static const cassandra::Query kQuery{"SELECT id, name FROM users WHERE id = ?"};

auto result = session->Execute(cassandra::Consistency::kQuorum, kQuery, 42);
auto row    = result.AsSingleRow<UserRow>(cassandra::io::kRowTag);
LOG_INFO("{} {}", row.id, row.name);
```

---

### INSERT с параметрами

**cpv-cql-driver**
```cpp
auto cmd = cql::Command("INSERT INTO users (id, name) VALUES (?, ?)")
    .setConsistency(cql::ConsistencyLevel::Quorum)
    .addParameters(cql::Int(1), cql::Text("alice"));

session.execute(std::move(cmd)).then([] {
    std::cout << "inserted\n";
});
```

**userver-cql-driver**
```cpp
static const cassandra::Query kInsert{"INSERT INTO users (id, name) VALUES (?, ?)"};

session->Execute(cassandra::Consistency::kQuorum, kInsert, 1, std::string{"alice"});
```

---

### SELECT нескольких строк

**cpv-cql-driver**
```cpp
session.query(cql::Command("SELECT id, name FROM users")
    .setConsistency(cql::ConsistencyLevel::One))
.then([] (cql::ResultSet result) {
    cql::Int  id;
    cql::Text name;
    for (std::size_t i = 0; i < result.getRowsCount(); ++i) {
        result.fill(id, name);   // позиционное заполнение в цикле
        std::cout << id << " " << name << "\n";
    }
});
```

**userver-cql-driver**
```cpp
static const cassandra::Query kSelect{"SELECT id, name FROM users"};

struct UserRow { cassandra::io::Int id; std::string name; };

auto result = session->Execute(cassandra::Consistency::kOne, kSelect);
auto rows   = result.AsContainer<std::vector<UserRow>>(cassandra::io::kRowTag);

for (const auto& row : rows) {
    LOG_DEBUG("{} {}", row.id, row.name);
}
```

---

### Batch-операции

**cpv-cql-driver**
```cpp
auto batch = cql::BatchCommand()
    .addQuery("INSERT INTO users (id, name) VALUES (?, ?)")
    .openParameterSet().addParameters(cql::Int(1), cql::Text("alice"))
    .addQuery("INSERT INTO users (id, name) VALUES (?, ?)")
    .openParameterSet().addParameters(cql::Int(2), cql::Text("bob"));

session.execute(std::move(batch)).then([] {
    std::cout << "batch done\n";
});
```

**userver-cql-driver**
```cpp
static const cassandra::Query kInsert{"INSERT INTO users (id, name) VALUES (?, ?)"};

cassandra::BatchQueryStore batch(cassandra::Consistency::kQuorum);
batch.AddQuery(kInsert, 1, std::string{"alice"});
batch.AddQuery(kInsert, 2, std::string{"bob"});

session->BatchExecute(batch);
```

---

### Итоговое сравнение

| Характеристика | userver-cql-driver | cpv-cql-driver |
|----------------|-------------------|----------------|
| Базовый фреймворк | [userver](https://userver.tech/) | [Seastar](https://seastar.io/) |
| Модель асинхронности | Stackful-корутины (синхронный стиль) | Futures + `.then()` |
| Интеграция с компонентной системой | ✅ Нативный userver-компонент | ❌ Отсутствует |
| Конфигурация через secdist | ✅ Да | ❌ Нет |
| Извлечение строк | Типобезопасно через агрегатные структуры | Позиционное `result.fill(a, b, ...)` |
| Binding параметров | Variadic-templates `Execute(..., p1, p2)` | Builder `.addParameters(p1, p2)` |
| Batch-операции | ✅ `BatchQueryStore` + `BatchExecute` | ✅ `BatchCommand` |
| Кэш prepared statements | ✅ NWayLRU, настраивается | ✅ Есть |
| Пул соединений | ✅ Настраиваемый min/max/TTL | ✅ Есть |
| Последний релиз | активная разработка | 2019 |
| Лицензия | Apache-2.0 | MIT |

## Лицензия

Распространяется под лицензией [Apache-2.0](http://www.apache.org/licenses/LICENSE-2.0).