#include <cassandra/io/buffer_io.hpp>
#include <cassandra/io/integral_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstdint>

namespace cassandra::io::protocol {
void FrameHeader::Serialize(RawBuffer& buffer) const {
    buffer.reserve(buffer.size() + kHeaderSize);

    WriteBuffer(buffer, version);
    WriteBuffer(buffer, flags);
    WriteBuffer(buffer, stream);
    WriteBuffer(buffer, static_cast<uint8_t>(opcode));
    WriteBuffer(buffer, length);
}

void FrameHeader::UpdateLength(RawBuffer& buffer, int32_t body_length) const {
    detail::WriteIntBE(buffer.data() + 5, buffer.data() + 9, body_length);
}

void FrameHeader::Parse(RawBufferView data) {
    size_t offset = 0;
    this->version = ReadBuffer<decltype(this->version)>(data, offset);
    this->flags = ReadBuffer<decltype(this->flags)>(data, offset);
    this->stream = ReadBuffer<decltype(this->stream)>(data, offset);
    this->opcode = static_cast<Opcode>(ReadBuffer<uint8_t>(data, offset));
    this->length = ReadBuffer<decltype(this->length)>(data, offset);
}
}  // namespace cassandra::io::protocol
