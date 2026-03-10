#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
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
        options[String("CQL_VERSION")] = String("3.0.0");
    }
    StartupMessage(std::string_view compression_protocol) : RequestMessage(FrameHeader{.opcode = Opcode::kStartup}) {
        options[String("CQL_VERSION")] = String("3.0.0");
        options[String("COMPRESSION")] = String(compression_protocol.data(), compression_protocol.size());
    }

    void Serialize(RawBuffer& buffer) override {
        // Serialize the message body
        std::vector<std::byte> body_buffer;
        BufferWriter writer(body_buffer);
        writer.Write(options);

        // serialize message header
        _header.length = body_buffer.size();
        _header.Serialize(buffer);
        // concat body buffer to the end of the buffer
        for (auto b : body_buffer) {
            buffer.push_back(b);
        }
    }

private:
    StringMap options;
};
}  // namespace cassandra::io::protocol
