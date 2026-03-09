#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <string>

namespace cassandra::io::protocol {

class RequestMessage : public Message {
public:
    RequestMessage(FrameHeader&& frame) : Message(std::move(frame)) {}

    virtual void Serialize(std::string& buffer) const = 0;
};

class OptionsMessage final : public RequestMessage {
public:
    OptionsMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kOptions}) {}

    void Serialize(std::string& buffer) const override {
        // Serialize the message header
        _header.Serialize(buffer);
    }
};
}  // namespace cassandra::io::protocol
