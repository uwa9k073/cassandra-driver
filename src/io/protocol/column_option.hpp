#pragma once

#include <memory>
#include <vector>

#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/types.hpp>

namespace cassandra::io::protocol {
class CustomPayload;
class ListPayload;   // includes ColumnOption
class SetPayload;    // includes ColumnOption
class MapPayload;    // includes ColumnOption
class TuplePayload;  // includes ColumnOption
class UdtPayload;    // includes ColumnOption

class ColumnPayload {
public:
    virtual ~ColumnPayload() = default;
    virtual void Parse(BufferReader& reader) = 0;
};

class ColumnOption {
public:
    void Parse(BufferReader& reader);

    ColumnType GetColumnType() const { return _type; }
    ColumnPayload* GetPayloadPtr() const { return _value.get(); }
    bool HasPayload() const { return _value != nullptr; }
    template <typename T>
    T* GetPayloadPtrAs() const {
        return dynamic_cast<T*>(_value.get());
    }
    template <typename T>
    const T* GetPayloadPtrAs() const {
        return dynamic_cast<const T*>(_value.get());
    }

    template <typename T>
    T GetPayloadAs() const {
        return *dynamic_cast<T*>(_value.get());
    }

private:
    ColumnType _type;
    std::unique_ptr<ColumnPayload> _value = nullptr;
};

class ListPayload : public ColumnPayload {
public:
    void Parse(BufferReader& reader);

private:
    ColumnOption _element_type;
};
class SetPayload : public ColumnPayload {
public:
    void Parse(BufferReader& reader);

private:
    ColumnOption _element_type;
};
class MapPayload : public ColumnPayload {
public:
    void Parse(BufferReader& reader);

private:
    ColumnOption _key_type;
    ColumnOption _value_type;
};
class TuplePayload : public ColumnPayload {
public:
    using Types = std::vector<ColumnOption>;

    void Parse(BufferReader& reader);

private:
    Types _elements;
};
class UdtPayload : public ColumnPayload {
public:
    using FieldsType = std::vector<std::pair<String, ColumnOption>>;
    void Parse(BufferReader& reader);

private:
    String _keyspace;
    String _udt_name;
    FieldsType _fields;
};

class CustomPayload : public ColumnPayload {
public:
    void Parse(BufferReader& reader);

private:
    String _value;
};

}  // namespace cassandra::io::protocol
