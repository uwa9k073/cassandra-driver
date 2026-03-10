#pragma once

#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/string_types.hpp>
#include <cassandra/io/list_types.hpp>
#include <cstddef>
#include <span>
#include <vector>
namespace cassandra::io {
class BufferWriter {
public:
    explicit BufferWriter(std::vector<std::byte>& buffer) noexcept : buffer_(buffer) {}
    // template<class T>
    //
    // [[nodiscard]] size_t Size() const noexcept { return buffer_.size(); }
    // [[nodiscard]] std::span<std::byte> Data() noexcept { return buffer_; }
    // [[nodiscard]] std::span<const std::byte> Data() const noexcept { return buffer_; }

    // // ===== Reserve space (avoid reallocations) =====
    // void Reserve(size_t additional) { buffer_.reserve(buffer_.size() + additional); }

    // // ===== Primitive Writes =====
    // void WriteTinyInt(TinyInt value) { WriteIntBE<TinyInt>(value); }
    // void WriteSmallInt(SmallInt value) { WriteIntBE<SmallInt>(value); }
    // void WriteInt(Int value) { WriteIntBE<Int>(value); }
    // void WriteBigInt(BigInt value) { WriteIntBE<BigInt>(value); }

    // // ===== Cassandra Types =====
    // void WriteString(std::string_view value);
    // void WriteNullString();
    // void WriteBytes(std::span<const std::byte> value);
    // void WriteNullBytes();
    // void WriteStringList(const std::vector<std::string>& list);
    // void WriteStringMultiMap(const std::unordered_map<std::string, std::vector<std::string>>& map);

    // ===== Write raw span (zero-copy append) =====
    // void WriteRaw(std::span<const std::byte> data) { buffer_.insert(buffer_.end(), data.begin(), data.end()); }

    // template <std::integral T>
    // void WriteIntBE(T value) {
    //     detail::WriteIntBE(buffer_, value);
    // }

    template <class T>
    void Write(const T& value) {
        detail::Write(buffer_, value);
    }

    // template <class T>
    // void Write(T&& value) {
    //     detail::Write(buffer_, std::move(value));
    // }

private:
    std::vector<std::byte>& buffer_;
};
}  // namespace cassandra::io
