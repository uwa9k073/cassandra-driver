#pragma once

#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/io/row_types.hpp>
#include <string>
#include <userver/logging/log.hpp>

namespace cassandra {
class Row {
public:
    Row(std::vector<io::protocol::RawBuffer>&& buffer)
        : _columns(std::move(buffer)) {}
    Row(const std::vector<io::protocol::RawBuffer>& buffer) : _columns(buffer) {}

    template <class T>
    T As(io::FieldTag) const {
        // using ValueType = std::decay_t<T>;

        LOG_DEBUG(
            "BUFFER_STRING: {}",
            std::string{
                reinterpret_cast<const char*>(_columns.front().data()),
                _columns.front().size()
            }
        );
        io::BufferReader reader{_columns.front()};
        return reader.Read<T>();
    }
    template <class T>
    T As(io::RowTag tag) const;

private:
    std::vector<io::protocol::RawBuffer> _columns;

    template <typename T>
    void To(T&& val, io::FieldTag) const {
        using ValueType = std::decay_t<T>;

        // LOG_DEBUG("READING FIELD");

        io::BufferReader reader{_columns.front()};
        val = reader.Read<ValueType>();
    }
};
}  // namespace cassandra
