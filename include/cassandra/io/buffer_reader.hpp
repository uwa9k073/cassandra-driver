#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/string_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <span>
namespace cassandra::io {
class BufferReader {
public:
    explicit BufferReader(std::span<const std::byte> data) noexcept : data_(data), offset_(0) {}

    // ===== Core API =====
    [[nodiscard]] size_t Offset() const noexcept { return offset_; }
    [[nodiscard]] size_t Remaining() const noexcept { return data_.size() - offset_; }
    [[nodiscard]] bool Empty() const noexcept { return offset_ >= data_.size(); }

    // [[nodiscard]] std::vector<std::string> ReadStringList() {
    //     auto value_count = ReadIntBE<Short>();
    //     std::vector<std::string> values;
    //     values.reserve(value_count);
    //     for (decltype(value_count) i = 0; i < value_count; ++i) {
    //         values.push_back(ReadString());
    //     }

    //     return values;
    // }
    // [[nodiscard]] std::unordered_map<std::string, std::vector<std::string>> ReadStringMultiMap() {
    //     // count of pairs
    //     auto count = ReadSmallInt();

    //     std::unordered_map<std::string, std::vector<std::string>> result;
    //     result.reserve(count);

    //     for (SmallInt i = 0; i < count; ++i) {
    //         std::string key = ReadString();
    //         auto values = ReadStringList();
    //         result.emplace(std::move(key), std::move(values));
    //     }

    //     return result;
    // }

    template <class T>
    [[nodiscard]] T Read() {
        return detail::Read<T>(this->data_, this->offset_);
    }

private:
    std::span<const std::byte> data_;
    size_t offset_ = 0;
};
}  // namespace cassandra::io
