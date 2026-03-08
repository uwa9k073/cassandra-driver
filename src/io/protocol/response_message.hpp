#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cstdint>


namespace cassandra::io::protocol {
class ResponseMessage : public Message{
    public:
    ResponseMessage(void* data) : Message(DeserializeHeader(data)) {};

    static FrameHeader DeserializeHeader(void* data){
        FrameHeader header;
        header.Deserialize(reinterpret_cast<uint8_t*>(data));
        return header;
    }
    
    static void DeserializeBody(void* data);
};
}
