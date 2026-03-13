#pragma once

#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cstddef>
#include <cstdint>

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

enum class ColumnType : Short {
    kCustom = 0x0000,
    kAscii = 0x0001,
    kBigInt = 0x0002,
    kBlob = 0x0003,
    kBoolean = 0x0004,
    kCounter = 0x0005,
    kDecimal = 0x0006,
    kDouble = 0x0007,
    kFloat = 0x0008,
    kInt = 0x0009,
    kTimestamp = 0x000B,
    kUUID = 0x000C,
    kVarChar = 0x000D,
    kVarInt = 0x000E,
    kTimeUUID = 0x000F,
    kInet = 0x0010,
    kDate = 0x0011,
    kTime = 0x0012,
    kSmallInt = 0x0013,
    kTinyInt = 0x0014,
    kList = 0x0020,
    kMap = 0x0021,
    kSet = 0x0022,
    kUDT = 0x0030,
    kTuple = 0x0031
};

/// CQL Protocol Version 4 frame header format
struct FrameHeader {
    static constexpr std::size_t kHeaderSize =
        9;  // version(1) + flags(1) + stream(2) +
            // opcode(1) + length(4)

    uint8_t version{(uint8_t)MessageDirection::kRequest};  // Protocol version
    uint8_t flags{0};                                      // Frame flags
    int16_t stream{0};                                     // Stream identifier
    Opcode opcode{0};                                      // Operation code
    int32_t length{0};                                     // Body length

    void Serialize(RawBuffer& buffer) const;
    void UpdateLength(RawBuffer& buffer, int32_t body_length) const;
    void Parse(RawBufferView data);
};
}  // namespace cassandra::io::protocol
