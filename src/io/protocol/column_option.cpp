#include <cassandra/io/protocol/frame.hpp>
#include <io/protocol/column_option.hpp>
#include <memory>

namespace cassandra::io::protocol {

void ColumnOption::Parse(BufferReader<BufferView>& reader) {
    _type = static_cast<ColumnType>(reader.Read<Short>());
    switch (_type) {
        case ColumnType::kCustom:
            _value = std::make_unique<CustomPayload>();
            break;
        case ColumnType::kList:
            _value = std::make_unique<ListPayload>();
            break;
        case ColumnType::kMap:
            _value = std::make_unique<MapPayload>();
            break;
        case ColumnType::kSet:
            _value = std::make_unique<SetPayload>();
            break;
        case ColumnType::kUDT:
            _value = std::make_unique<UdtPayload>();
            break;
        case ColumnType::kTuple:
            _value = std::make_unique<TuplePayload>();
            break;
        default:
            _value = nullptr;
            return;
    }

    _value->Parse(reader);
}

void ListPayload::Parse(BufferReader<BufferView>& reader) {
    _element_type.Parse(reader);
}

void CustomPayload::Parse(BufferReader<BufferView>& reader) {
    _value = reader.Read<String>();
}

void MapPayload::Parse(BufferReader<BufferView>& reader) {
    _key_type.Parse(reader);
    _value_type.Parse(reader);
}

void SetPayload::Parse(BufferReader<BufferView>& reader) {
    _element_type.Parse(reader);
}

void UdtPayload::Parse(BufferReader<BufferView>& reader) {
    _keyspace = reader.Read<String>();
    _udt_name = reader.Read<String>();

    auto count = reader.Read<Short>();
    _fields.reserve(count);
    for (Short i = 0; i < count; ++i) {
        _fields.emplace_back(reader.Read<String>(), ColumnOption());
        _fields.back().second.Parse(reader);
    }
}

void TuplePayload::Parse(BufferReader<BufferView>& reader) {
    auto count = reader.Read<Short>();

    _elements.reserve(count);

    for (Short i = 0; i < count; ++i) {
        ColumnOption element;
        element.Parse(reader);
        _elements.emplace_back(std::move(element));
    }
}
}  // namespace cassandra::io::protocol
