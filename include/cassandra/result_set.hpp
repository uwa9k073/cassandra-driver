#pragma once

#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <cassandra/row.hpp>
#include <userver/logging/log.hpp>
#include <cassandra/detail/result_wrapper.hpp>
namespace cassandra {
class ResultSet {
public:
    bool Empty() const { return !_pimpl || _pimpl->Empty(); }

    auto RowsAffected() const { return _pimpl->RowsAffected(); }

    auto ColumnsAffected() const { return _pimpl->ColumnsAffected(); }

    ResultSet(std::shared_ptr<detail::ResultWrapper> pimpl) : _pimpl(pimpl) {}

    template <class T>
    T AsSingleRow(io::FieldTag tag) const {
        return Front().As<T>(tag);
    }

    template <class T>
    T AsSingleRow(io::RowTag tag) const {
        return Front().As<T>(tag);
    }

    template <class Container>
    Container AsContainer(io::RowTag tag) const {
        auto content = _pimpl->RowsContentView();
        using ElementType = typename Container::value_type;
        Container result;
        result.reserve(content.size());
        for (const auto& row : content) {
            result.push_back(Row(row, _pimpl->ColumnsAffected()).As<ElementType>(tag)
            );
        }
        return result;
    }

    Row Front() const {
        return Row(_pimpl->RowsContentView().front(), _pimpl->ColumnsAffected());
    }

private:
    std::shared_ptr<detail::ResultWrapper> _pimpl;
};
}  // namespace cassandra
