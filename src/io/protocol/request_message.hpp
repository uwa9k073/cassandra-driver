#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstdint>
#include <unordered_map>
#include <userver/logging/log.hpp>

namespace cassandra::io::protocol {
class RequestMessage : public Message {
public:
    RequestMessage(FrameHeader&& frame) : Message(std::move(frame)) {}

    void Serialize(RawBuffer& buffer, Lz4Compressor* compressor_ptr = nullptr) {
        if (!compressor_ptr) {
            // Serialize the message header
            _header.Serialize(buffer);
            // Serialize the message body
            SerializeBody(buffer);
        } else {
            _header.flags |= static_cast<uint8_t>(Flags::kCompression);

            // Serialize the message header
            _header.Serialize(buffer);

            RawBuffer body_buffer;
            // Serialize the message body into a separate buffer
            SerializeBody(body_buffer);
            // Compress the body
            compressor_ptr->Compress(body_buffer, buffer);
        }
        // write body length if length is not null
        if (int32_t body_length = buffer.size() - FrameHeader::kHeaderSize;
            body_length > 0) {
            _header.UpdateLength(buffer, body_length);
        }
    }
    // we only need to implement SerializeBody in derived classes,
    // and it should append to the buffer and rewrite the header
    // length if necessary
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
    StartupMessage(std::string_view compression_protocol)
        : RequestMessage(FrameHeader{.opcode = Opcode::kStartup}) {
        options[String("CQL_VERSION")] = String("3.0.0");
        options[String("COMPRESSION")] =
            String(compression_protocol.data(), compression_protocol.size());
    }

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(options);
    }

private:
    StringMap options;
};

class QueryMessage final : public RequestMessage {
public:
    QueryMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kQuery}) {}

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(query);
    }

private:
    LongString query;
};

}  // namespace cassandra::io::protocol
