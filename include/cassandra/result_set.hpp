#pragma once

#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <cassandra/row.hpp>
#include <userver/logging/log.hpp>
#include <vector>

namespace cassandra {
class ResultSet {
public:
    bool Empty() const { return _rows_content.empty(); }

    auto RowsAffected() const { return _columns_count; }

    auto ColumnsAffected() const { return _rows_count; }

    ResultSet() : _rows_content({}), _columns_count(0), _rows_count(0){};

    ResultSet(
        std::vector<io::protocol::BytesBuffer>&& rows,
        io::Int columns_count,
        io::Int rows_count
    )
        : _rows_content(rows),
          _columns_count(columns_count),
          _rows_count(rows_count) {}

    template <class T>
    T AsSingleRow(io::FieldTag tag) const {
        return Front().As<T>(tag);
    }

    template <class T>
    T AsSingleRow(io::RowTag tag) const {
        LOG_DEBUG(
            "BUFFER_DATA_ROW_TAG: ",
            std::string{
                reinterpret_cast<const char*>(_rows_content.data()),
                _rows_content.size()
            }
        );
        return Front().As<T>(tag);
    }

    Row Front() const { return Row(_rows_content.front(), _columns_count); }

private:
    std::vector<io::protocol::BytesBuffer> _rows_content;

    io::Int _columns_count;
    io::Int _rows_count;
};
}  // namespace cassandra
