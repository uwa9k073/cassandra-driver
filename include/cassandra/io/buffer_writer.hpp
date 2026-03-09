#pragma once

#include <cstddef>
#include <span>
#include <unordered_map>
#include <vector>
#include "cassandra/io/cassandra_types.hpp"
namespace cassandra::io {
class BufferWriter {
    public:
    explicit BufferWriter(std::vector<std::byte>& buffer) noexcept : buffer_(buffer) {}
    // template<class T>
    //
    [[nodiscard]] size_t Size() const noexcept { return buffer_.size(); }
    [[nodiscard]] std::span<std::byte> Data() noexcept { return buffer_; }
    [[nodiscard]] std::span<const std::byte> Data() const noexcept { return buffer_; }

    // ===== Reserve space (avoid reallocations) =====
    void Reserve(size_t additional) { buffer_.reserve(buffer_.size() + additional); }

    // ===== Primitive Writes =====
    void WriteTinyInt(TinyInt value) { WriteIntBE<TinyInt>(value); }
    void WriteSmallInt(SmallInt value) { WriteIntBE<SmallInt>(value); }
    void WriteInt(Int value) { WriteIntBE<Int>(value); }
    void WriteBigInt(BigInt value) { WriteIntBE<BigInt>(value); }

    // ===== Cassandra Types =====
    void WriteString(std::string_view value);
    void WriteNullString();
    void WriteBytes(std::span<const std::byte> value);
    void WriteNullBytes();
    void WriteStringList(const std::vector<std::string>& list);
    void WriteStringMultiMap(const std::unordered_map<std::string, std::vector<std::string>>& map);

    // ===== Write raw span (zero-copy append) =====
    void WriteRaw(std::span<const std::byte> data) { buffer_.insert(buffer_.end(), data.begin(), data.end()); }

    template <std::integral T>
    void WriteIntBE(T value) {
        for (size_t i = 0; i < sizeof(T); ++i) {
            buffer_.push_back(static_cast<std::byte>((value >> (8 * (sizeof(T) - 1 - i))) & 0xFF));
        }
    }

private:
    std::vector<std::byte>& buffer_;
};
}  // namespace cassandra::io
