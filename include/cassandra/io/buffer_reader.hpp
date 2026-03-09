#pragma once

#include <iterator>
#include <span>
#include <unordered_map>
#include "cassandra/io/cassandra_types.hpp"
namespace cassandra::io {

class BufferReader {
public:
    // Accept any contiguous byte buffer (vector, array, etc.)
    // template <typename T>
    // requires std::contiguous_iterator<typename std::decay_t<T>::iterator> &&
    //          (sizeof(typename std::decay_t<T>::value_type) == 1)
    // explicit BufferReader(T&& buffer) noexcept : data_(std::as_bytes(std::span<std::decay_t<T>>(buffer))) {}

    explicit BufferReader(std::span<const std::byte> data) noexcept : data_(data), offset_(0) {}

    // ===== Core API =====
    [[nodiscard]] size_t Offset() const noexcept { return offset_; }
    [[nodiscard]] size_t Remaining() const noexcept { return data_.size() - offset_; }
    [[nodiscard]] bool Empty() const noexcept { return offset_ >= data_.size(); }

    // ===== Read Raw Span (zero-copy view) =====
    [[nodiscard]] std::span<const std::byte> ReadSpan(size_t len) {
        EnsureSize(len);
        auto result = data_.subspan(offset_, len);
        offset_ += len;
        return result;
    }

    // ===== Primitive Reads =====
    [[nodiscard]] TinyInt ReadTinyInt() { return ReadIntBE<TinyInt>(); }
    [[nodiscard]] SmallInt ReadSmallInt() { return ReadIntBE<SmallInt>(); }
    [[nodiscard]] Int ReadInt() { return ReadIntBE<Int>(); }
    [[nodiscard]] BigInt ReadBigInt() { return ReadIntBE<BigInt>(); }

    // ===== Cassandra Types =====
    [[nodiscard]] std::string ReadString() {
        auto len = ReadSmallInt();
        std::string res(reinterpret_cast<const char*>(data_.data()) + offset_, len);
        offset_ += len;
        return res;
    }
    [[nodiscard]] std::optional<std::span<const std::byte>> ReadBytes();
    [[nodiscard]] std::vector<std::string> ReadStringList();
    [[nodiscard]] std::unordered_map<std::string, std::vector<std::string>> ReadStringMultiMap() {
        // count of pairs
        auto count = ReadSmallInt();

        std::unordered_map<std::string, std::vector<std::string>> result;
        result.reserve(count);

        for (SmallInt i = 0; i < count; ++i) {
            std::string key = ReadString();
            auto value_count = ReadSmallInt();
            std::vector<std::string> values(value_count);
            for (SmallInt j = 0; j < value_count; ++j) {
                values.push_back(ReadString());
            }
            result.emplace(std::move(key), std::move(values));
        }

        return result;
    }

private:
    void EnsureSize(size_t required) const {
        if (offset_ + required > data_.size()) {
            throw std::runtime_error("Buffer underread");
        }
    }

    template <std::integral T>
    [[nodiscard]] T ReadIntBE() {
        EnsureSize(sizeof(T));
        T value = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            value = (value << 8) | static_cast<std::make_unsigned_t<T>>(data_[offset_ + i]);
        }
        offset_ += sizeof(T);
        return value;
    }

    std::span<const std::byte> data_;
    size_t offset_ = 0;
};
}  // namespace cassandra::io
