#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/io/string_types.hpp>
#include <span>
#include <string>
#include <unordered_map>
#include <userver/logging/log.hpp>
#include <vector>

namespace cassandra::io::protocol {
class ResponseMessage : public Message {
public:
    ResponseMessage(FrameHeader&& header) : Message(std::move(header)){};

    static FrameHeader DeserializeHeader(std::span<std::byte> data) {
        FrameHeader header;
        header.Deserialize(data);
        return header;
    }

    virtual void DeserializeBody(std::span<std::byte> data_buffer) = 0;
};

class SupportMessage : public ResponseMessage {
public:
    SupportMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    std::unordered_map<std::string, std::vector<std::string>> GetOptions() const { return options; }

    void DeserializeBody(std::span<std::byte> buffer) override { options = BufferReader{buffer}.ReadStringMultiMap(); }

private:
    std::unordered_map<std::string, std::vector<std::string>> options;
};
}  // namespace cassandra::io::protocol
