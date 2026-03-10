#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <span>
#include <userver/logging/log.hpp>

namespace cassandra::io::protocol {
class ResponseMessage : public Message {
public:
    ResponseMessage(FrameHeader&& header) : Message(std::move(header)){};

    static FrameHeader ParseHeader(RawBufferView data) {
        FrameHeader header;
        header.Parse(data);
        return header;
    }

    virtual void ParseBody(RawBufferView data_buffer) = 0;
};

class ErrorMessage : public ResponseMessage {
public:
    ErrorMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    void ParseBody(RawBufferView buffer) override {
        auto reader = BufferReader{buffer};
        error_code = reader.Read<Int>();
        error_message = reader.Read<String>();
    }

private:
    Int error_code;
    String error_message;
};

class ReadyMessage : public ResponseMessage {
public:
    ReadyMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    // Ready message does not have a body
    void ParseBody(RawBufferView /*buffer*/) override {}
};

class AuthentificateMessage : public ResponseMessage {
public:
    AuthentificateMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    void ParseBody(RawBufferView buffer) override {
        auth_challenge = BufferReader{buffer}.Read<String>();
        LOG_DEBUG("Auth challenge: {}", auth_challenge.GetUnderlying());
    }

private:
    String auth_challenge;
};

class SupportMessage : public ResponseMessage {
public:
    SupportMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    StringMultiMap GetOptions() const { return options; }

    void ParseBody(RawBufferView buffer) override { options = BufferReader{buffer}.Read<StringMultiMap>(); }

private:
    StringMultiMap options;
};
}  // namespace cassandra::io::protocol
