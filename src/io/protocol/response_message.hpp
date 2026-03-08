#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cstdint>


namespace cassandra::io::protocol {
class ResponseMessage : public Message{
    public:
    ResponseMessage(std::string_view data) : Message(DeserializeHeader(data)) {};

    static FrameHeader DeserializeHeader(std::string_view data){
        FrameHeader header;
        header.Deserialize(reinterpret_cast<const uint8_t*>(data.data()));
        return header;
    }

    static void DeserializeBody(std::string_view data);
};
}
