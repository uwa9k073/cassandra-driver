#pragma once

#include "cassandra/io/buffer_reader.hpp"
#include "cassandra/io/protocol/types.hpp"
#include "cassandra/io/row_types.hpp"
namespace cassandra {
class Row {
public:

    Row(io::protocol::RawBuffer&& buffer)
        : _columns_buffer(std::move(buffer)) {}

    template <class T>
    T As(io::FieldTag tag) const {
        T val;
        To(val, tag);
        return val;
    }
    template <class T>
    T As(io::RowTag tag) const;

private:
    io::protocol::RawBuffer _columns_buffer;

    template <typename T>
    void To(T&& val, io::FieldTag) const {
        using ValueType = std::decay_t<T>;

        static_assert(sizeof(ValueType)==_columns_buffer.size(), "ValueType size does not match buffer size");

        io::BufferReader reader{_columns_buffer};
        val = reader.Read<ValueType>();
    }
};
}  // namespace cassandra
