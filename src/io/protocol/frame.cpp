#include <cassandra/io/protocol/frame.hpp>
#include <cstdint>
#include "cassandra/io/buffer_io_base.hpp"

namespace cassandra::io::protocol {
void FrameHeader::Serialize(RawBuffer& buffer) const {
    buffer.reserve(buffer.size() + kHeaderSize);

    detail::WriteIntBE(buffer, version);
    detail::WriteIntBE(buffer, flags);
    detail::WriteIntBE(buffer, stream);
    detail::WriteIntBE(buffer, static_cast<uint8_t>(opcode));
    detail::WriteIntBE(buffer, length);
}


void FrameHeader::UpdateLength(RawBuffer& buffer, int32_t body_length) const {
    detail::WriteIntBE(buffer.data()+5, buffer.data()+9, body_length);
}

void FrameHeader::Parse(RawBufferView data) {
    size_t offset = 0;
    this->version = detail::ReadIntBE<decltype(this->version)>(data, offset);
    this->flags = detail::ReadIntBE<decltype(this->flags)>(data, offset);
    this->stream = detail::ReadIntBE<decltype(this->stream)>(data, offset);
    this->opcode = static_cast<Opcode>(detail::ReadIntBE<uint8_t>(data, offset));
    this->length = detail::ReadIntBE<decltype(this->length)>(data, offset);
}
}  // namespace cassandra::io::protocol
