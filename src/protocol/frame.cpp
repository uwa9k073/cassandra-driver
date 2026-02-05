#include <cassandra/protocol/frame.hpp>

namespace cassandra::protocol {
void FrameHeader::Serialize(std::vector<uint8_t>& buffer) const {
    buffer.push_back(version);
    buffer.push_back(flags);
    buffer.push_back(static_cast<uint8_t>((stream >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(stream & 0xFF));
    buffer.push_back(opcode);
    buffer.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(length & 0xFF));
}

void FrameHeader::Deserialize(const uint8_t* data) {
    version = data[0];
    flags = data[1];
    stream = (static_cast<int16_t>(data[2]) << 8) | data[3];
    opcode = data[4];
    length = (static_cast<int32_t>(data[5]) << 24) | (static_cast<int32_t>(data[6]) << 16) |
             (static_cast<int32_t>(data[7]) << 8) | data[8];
}
}  // namespace cassandra::protocol
