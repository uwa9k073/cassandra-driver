#pragma once

#include <cassandra/cassandra_fwd.hpp>
#include <cassandra/io/bytes.hpp>
#include <cassandra/io/protocol/types.hpp>

namespace cassandra::detail {
class ResultWrapper {
public:
    ResultWrapper(
        std::vector<io::protocol::BytesBuffer>&& rows_content,
        io::Int columns_count,
        io::Int rows_count
    )
        : _rows_content(std::move(rows_content)),
          _columns_count(columns_count),
          _rows_count(rows_count) {}

    bool Empty() const { return _rows_content.empty(); }

    auto RowsAffected() const { return _rows_count; }

    auto ColumnsAffected() const { return _columns_count; }

    std::span<const io::protocol::BytesBuffer> RowsContentView() const {
        return _rows_content;
    }

private:
    std::vector<io::protocol::BytesBuffer> _rows_content;

    io::Int _columns_count;
    io::Int _rows_count;
};
}  // namespace cassandra::detail
