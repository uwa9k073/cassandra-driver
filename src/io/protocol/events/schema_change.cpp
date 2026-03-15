#include <io/protocol/events/schema_change.hpp>

namespace cassandra::io::protocol::events {

SchemaChange SchemaChange::Parse(BufferReader<BufferView>& reader) {
    SchemaChange change;
    change.change_type = reader.Read<String>();
    change.target = reader.Read<String>();

    if (change.target.GetUnderlying() == "KEYSPACE") {
        Keyspace keyspace;
        keyspace.value = reader.Read<String>();
        change.target_options = keyspace;
    } else if (change.target.GetUnderlying() == "TABLE" ||
               change.target.GetUnderlying() == "TYPE") {
        Table table;
        table.keyspace = reader.Read<String>();
        table.target_name = reader.Read<String>();
        change.target_options = table;
    } else if (change.change_type.GetUnderlying() == "FUNCTION" ||
               change.target.GetUnderlying() == "AGGREGATE") {
        Function function;
        function.keyspace = reader.Read<String>();
        function.function_name = reader.Read<String>();
        function.args = reader.Read<StringList>();
        change.target_options = function;
    }
    return change;
}

}  // namespace cassandra::io::protocol::events
