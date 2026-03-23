#pragma once

#include <cassandra/io/protocol/frame.hpp>

namespace cassandra::io::protocol {

enum class CompressionType {
    kNone,
    kLz4,
};
/// Base class for all CQL protocol messages
class Message {
public:
    virtual ~Message() = default;
    virtual Opcode GetOpcode() const { return _header.opcode; }
    const FrameHeader& GetHeader() const { return _header; }

    Message(FrameHeader&& header) : _header(std::move(header)) {}
    Message(const Message&) = delete;
    Message(Message&&) = default;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = default;

    void SetStreamId(std::int16_t id) { _header.stream = id; }
    std::int16_t GetStreamId() const { return _header.stream; }

protected:
    FrameHeader _header;
};

}  // namespace cassandra::io::protocol
