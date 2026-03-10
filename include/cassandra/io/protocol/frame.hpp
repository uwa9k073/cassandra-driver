#pragma once

#include <cstddef>
#include <cstdint>
#include <cassandra/io/protocol/types.hpp>

namespace cassandra::io::protocol {
/// CQL Protocol flags
enum class Flags : uint8_t {
    kCompression = 0x01,
    kTracing = 0x02,
    kCustomPayload = 0x04,
    kWarning = 0x08,
    kUseBeta = 0x10
};

/// CQL Protocol operation codes
enum class Opcode : uint8_t {
    kError = 0x00,
    kStartup = 0x01,
    kReady = 0x02,
    kAuthenticate = 0x03,
    kCredentials = 0x04,
    kOptions = 0x05,
    kSupported = 0x06,
    kQuery = 0x07,
    kResult = 0x08,
    kPrepare = 0x09,
    kExecute = 0x0A,
    kRegister = 0x0B,
    kEvent = 0x0C,
    kBatch = 0x0D,
    kAuthChallenge = 0x0E,
    kAuthResponse = 0x0F,
    kAuthSuccess = 0x10
};

/// Result kind codes
enum class ResultKind : int32_t {
    kVoid = 0x0001,
    kRows = 0x0002,
    kSetKeyspace = 0x0003,
    kPrepared = 0x0004,
    kSchemaChange = 0x0005
};

enum class MessageDirection : uint8_t { kRequest = 0x04, kResponse = 0x84 };

/// CQL Protocol Version 4 frame header format
struct FrameHeader {
    static constexpr std::size_t kHeaderSize = 9;  // version(1) + flags(1) + stream(2) + opcode(1) + length(4)

    uint8_t version{(uint8_t)MessageDirection::kRequest};  // Protocol version
    uint8_t flags{0};                                      // Frame flags
    int16_t stream{0};                                     // Stream identifier
    Opcode opcode{0};                                      // Operation code
    int32_t length{0};                                     // Body length

    void Serialize(RawBuffer& buffer) const;
    void Parse(RawBufferView data);
};
}  // namespace cassandra::io::protocol
