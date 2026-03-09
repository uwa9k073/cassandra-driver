#include <cassandra/io/protocol/frame.hpp>
#include <cstdint>
#include "cassandra/io/buffer_reader.hpp"

namespace cassandra::io::protocol {
void FrameHeader::Serialize(RawBuffer& buffer) const {
    buffer.reserve(buffer.size() + kHeaderSize);

    buffer.push_back(static_cast<std::byte>(version));
    buffer.push_back(static_cast<std::byte>(flags));

    // Big-endian для stream (int16_t)
    buffer.push_back(static_cast<std::byte>((stream >> 8) & 0xFF));
    buffer.push_back(static_cast<std::byte>(stream & 0xFF));

    buffer.push_back(static_cast<std::byte>(opcode));

    // Big-endian для length (int32_t)
    buffer.push_back(static_cast<std::byte>((length >> 24) & 0xFF));
    buffer.push_back(static_cast<std::byte>((length >> 16) & 0xFF));
    buffer.push_back(static_cast<std::byte>((length >> 8) & 0xFF));
    buffer.push_back(static_cast<std::byte>(length & 0xFF));
}

void FrameHeader::Parse(RawBufferView data) {
    BufferReader reader(data);
    this->version = reader.ReadIntBE<decltype(this->version)>();
    this->flags = reader.ReadIntBE<decltype(this->flags)>();
    this->stream = reader.ReadIntBE<decltype(this->stream)>();
    this->opcode = static_cast<Opcode>(reader.ReadIntBE<uint8_t>());
    this->length = reader.ReadIntBE<decltype(this->length)>();
}
}  // namespace cassandra::io::protocol
