#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <string>
#include <unordered_map>

namespace cassandra::io::protocol {

class RequestMessage : public Message {
public:
    RequestMessage(FrameHeader&& frame) : Message(std::move(frame)) {}

    virtual void Serialize(RawBuffer& buffer) = 0;
};

class OptionsMessage final : public RequestMessage {
public:
    OptionsMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kOptions}) {}

    void Serialize(RawBuffer& buffer) override {
        // Serialize the message header
        _header.Serialize(buffer);
    }
};


class StartupMessage final : public RequestMessage {
public:
    StartupMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kStartup}) {
        options["CQL_VERSION"] = "3.0.0";
    }
    StartupMessage(std::string_view compression_protocol) : RequestMessage(FrameHeader{.opcode = Opcode::kStartup}) {
        options["CQL_VERSION"] = "3.0.0";
        options["COMPRESSION"] = std::string(compression_protocol);
    }

    void Serialize(RawBuffer& buffer) override {
        // Serialize the message header
        _header.Serialize(buffer);

        // update body length cause we don't know the final body size on the first pass
        _header.length = 20102;
        _header.Serialize(buffer);

    }


private:
std::unordered_map<std::string, std::string> options;
};
}  // namespace cassandra::io::protocol
