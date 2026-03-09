#include <cassandra/io/protocol/frame.hpp>
#include <cstdint>

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

void FrameHeader::Deserialize(RawBufferView data) {
    this->version = static_cast<uint8_t>(data[0]);
    this->flags = static_cast<uint8_t>(data[1]);
    this->stream = (static_cast<int16_t>(data[3]) << 8) | static_cast<uint8_t>(data[2]);
    this->opcode = static_cast<Opcode>(data[4]);
    this->length = (static_cast<int32_t>(data[5]) << 24) | (static_cast<int32_t>(data[6]) << 16) |
                   (static_cast<int32_t>(data[7]) << 8) | static_cast<int32_t>(data[8]);
}
}  // namespace cassandra::io::protocol
