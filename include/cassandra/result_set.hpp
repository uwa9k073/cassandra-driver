#pragma once

#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <cassandra/row.hpp>
#include <type_traits>

namespace cassandra {
class ResultSet {
public:
    bool Empty() const { return _rows_content.empty(); }
    auto RowsAffected() const { return _columns_count; }

    auto ColumnsAffected() const { return _rows_count; }

    ResultSet(
        const io::protocol::RawBuffer& rows,
        io::Int columns_count,
        io::Int rows_count
    )
        : _rows_content(rows),
          _columns_count(columns_count),
          _rows_count(rows_count) {}

    template <class T>
    T AsSingleRow(io::FieldTag) {
        LOG_DEBUG(
            "BUFFER_STRING: {}",
            std::string{
                reinterpret_cast<const char*>(_rows_content.data()),
                _rows_content.size()
            }
        );
        io::BufferReader reader(_rows_content);
        using ValueType = std::decay_t<T>;
        return reader.Read<ValueType>();
    }

private:
    io::protocol::RawBuffer _rows_content;

    io::Int _columns_count;
    io::Int _rows_count;
};
}  // namespace cassandra
