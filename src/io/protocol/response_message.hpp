#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cstdint>
#include <userver/logging/log.hpp>

namespace cassandra::io::protocol {
class ResponseMessage : public Message {
public:
    ResponseMessage(FrameHeader&& header) : Message(std::move(header)) {};

    static FrameHeader DeserializeHeader(std::string_view data) {
        FrameHeader header;
        header.Deserialize(reinterpret_cast<const uint8_t*>(data.data()));
        return header;
    }

    virtual void DeserializeBody(std::string_view data) = 0;
};

class SupportMessage : public ResponseMessage {
public:
    SupportMessage(FrameHeader&& header) : ResponseMessage(std::move(header)) {};

    void DeserializeBody(std::string_view data) override {
        body = std::string{data.data(), data.size()};
        LOG_DEBUG("CASSANDRA SUPPORTED BODY: {}", body);
    }

private:
    std::string body;
};
}  // namespace cassandra::io::protocol
