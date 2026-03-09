#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/io/string_types.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <userver/logging/log.hpp>
#include <vector>

namespace cassandra::io::protocol {
class ResponseMessage : public Message {
public:
    ResponseMessage(FrameHeader&& header) : Message(std::move(header)){};

    static FrameHeader DeserializeHeader(std::string_view data) {
        FrameHeader header;
        header.Deserialize(reinterpret_cast<const uint8_t*>(data.data()));
        return header;
    }

    virtual void DeserializeBody(std::span<char> data_buffer) = 0;
};

class SupportMessage : public ResponseMessage {
public:
    SupportMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    std::unordered_map<std::string, std::vector<std::string>> GetOptions() const { return options; }

    void DeserializeBody(std::span<char> buffer) override {
        size_t offset = 0;

        // Helper to read Big-Endian short (2 bytes)
        auto readShort = [&buffer, &offset]() -> uint16_t {
            uint16_t val = (static_cast<uint8_t>(buffer[offset]) << 8) | static_cast<uint8_t>(buffer[offset + 1]);
            offset += 2;
            return val;
        };

        // Helper to read string (length-prefixed)
        auto readString = [&buffer, &offset, &readShort]() -> std::string {
            uint16_t len = readShort();
            std::string str(buffer.data() + offset, len);
            offset += len;
            return str;
        };

        // 1. Read number of options
        uint16_t count = readShort();

        // 2. Read each key-value pair
        for (uint16_t i = 0; i < count; ++i) {
            std::string key = readString();

            // SUPPORTED values are lists of strings, not single strings!
            uint16_t value_count = readShort();
            std::vector<std::string> values;
            values.reserve(value_count);

            for (uint16_t j = 0; j < value_count; ++j) {
                values.push_back(readString());
            }

            options[key] = std::move(values);
        }
    }

private:
    std::unordered_map<std::string, std::vector<std::string>> options;
};
}  // namespace cassandra::io::protocol
