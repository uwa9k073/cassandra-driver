#pragma once

#include <cassandra/io/protocol/types.hpp>
#include <cassandra/row.hpp>
#include <vector>
#include <cassandra/io/row_types.hpp>

namespace cassandra {
class ResultSet {
public:
    bool Empty() const { return _rows.empty(); }
    auto RowsAffected() const { return _rows.size(); }

    ResultSet(std::vector<Row>&& rows): _rows(std::move(rows)) {}

    template<class T>
    T AsSingleRow(io::FieldTag tag){
        return _rows.front().As<T>(tag);
    }

private:
    std::vector<Row> _rows;
};
}  // namespace cassandra
