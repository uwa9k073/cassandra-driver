#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <span>
#include <string>
#include <unordered_map>
#include <userver/logging/log.hpp>
#include <vector>

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

class SupportMessage : public ResponseMessage {
public:
    SupportMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    StringMultiMap GetOptions() const { return options; }

    void ParseBody(RawBufferView buffer) override { options = BufferReader{buffer}.Read<StringMultiMap>(); }

private:
    StringMultiMap options;
};
}  // namespace cassandra::io::protocol
