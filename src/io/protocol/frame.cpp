#include <cassandra/io/protocol/frame.hpp>
#include <cstdint>
#include "cassandra/io/buffer_io_base.hpp"

namespace cassandra::io::protocol {
void FrameHeader::Serialize(RawBuffer& buffer) const {
    if (static_cast<std::size_t>(buffer.end() - buffer.begin()) < kHeaderSize) {
        buffer.reserve(buffer.size() + kHeaderSize);
    }

    detail::WriteIntBE(buffer.data(), buffer.data() + 1, version);
    detail::WriteIntBE(buffer.data() + 1, buffer.data() + 2, flags);
    detail::WriteIntBE(buffer.data() + 2, buffer.data() + 4, stream);
    detail::WriteIntBE(buffer.data() + 4, buffer.data() + 5, static_cast<uint8_t>(opcode));
    detail::WriteIntBE(buffer.data() + 5, buffer.data() + 9, length);
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
