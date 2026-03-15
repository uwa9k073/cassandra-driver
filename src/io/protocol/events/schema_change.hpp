#pragma once

#include <cassandra/io/buffer_reader.hpp>

namespace cassandra::io::protocol::events {

struct SchemaChange {
    struct Keyspace {
        String value;
    };

    struct Table {
        String keyspace;
        String target_name;
    };

    struct Function {
        String keyspace;
        String function_name;
        StringList args;
    };

    using TargetOptions = std::variant<Keyspace, Table, Function>;

    static SchemaChange Parse(BufferReader<BufferView>& reader);
    String change_type;
    String target;
    TargetOptions target_options;
};

}  // namespace cassandra::io::protocol::events
