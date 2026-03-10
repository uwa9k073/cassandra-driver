#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cstdint>
#include <unordered_map>
#include <userver/logging/log.hpp>
namespace cassandra::io::protocol {

class RequestMessage : public Message {
public:
    RequestMessage(FrameHeader&& frame) : Message(std::move(frame)) {}

    void Serialize(RawBuffer& buffer) {
        // Serialize the message header
        _header.Serialize(buffer);
        // Serialize the message body
        SerializeBody(buffer);
        // write body length if length is not null
        if (int32_t body_length = buffer.size() - FrameHeader::kHeaderSize; body_length > 0) {
            _header.UpdateLength(buffer, body_length);
        }
    }
    // we only need to implement SerializeBody in derived classes,
    // and it should append to the buffer and rewrite the header length if necessary
    virtual void SerializeBody(RawBuffer& buffer) = 0;
};

class OptionsMessage final : public RequestMessage {
public:
    OptionsMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kOptions}) {}
    void SerializeBody(RawBuffer& /*buffer*/) override {}
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

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(options);
    }

private:
    StringMap options;
};
}  // namespace cassandra::io::protocol
