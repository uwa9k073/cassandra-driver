#include <cassandra/io/protocol/frame.hpp>

namespace cassandra::io::protocol {
void FrameHeader::Serialize(std::string& buffer) const {
    buffer.reserve(buffer.size() + kHeaderSize);

    buffer.push_back(static_cast<char>(version));
    buffer.push_back(static_cast<char>(flags));

    // Big-endian для stream (int16_t)
    buffer.push_back(static_cast<char>((stream >> 8) & 0xFF));
    buffer.push_back(static_cast<char>(stream & 0xFF));

    buffer.push_back(static_cast<char>(opcode));

    // Big-endian для length (int32_t)
    buffer.push_back(static_cast<char>((length >> 24) & 0xFF));
    buffer.push_back(static_cast<char>((length >> 16) & 0xFF));
    buffer.push_back(static_cast<char>((length >> 8) & 0xFF));
    buffer.push_back(static_cast<char>(length & 0xFF));
}

void FrameHeader::Deserialize(const uint8_t* data) {
    version = data[0];
    flags = data[1];
    stream = (static_cast<int16_t>(data[2]) << 8) | data[3];
    opcode = static_cast<Opcode>(data[4]);
    length = (static_cast<int32_t>(data[5]) << 24) | (static_cast<int32_t>(data[6]) << 16) |
             (static_cast<int32_t>(data[7]) << 8) | data[8];
}
}  // namespace cassandra::io::protocol
