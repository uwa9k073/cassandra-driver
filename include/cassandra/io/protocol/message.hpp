#pragma once

#include <vector>

#include <cassandra/io/protocol/frame.hpp>

namespace cassandra::io::protocol {

/// Base class for all CQL protocol messages
class Message {
public:
    virtual ~Message() = default;
    virtual Opcode GetOpcode() const { return _header.opcode; }
    const FrameHeader& GetHeader() const { return _header; }

    Message(FrameHeader&& header) : _header(std::move(header)) {}
    Message(const Message&) = delete;
    Message(Message&&) = delete;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = delete;

protected:
    FrameHeader _header;
};

}  // namespace cassandra::io::protocol
