#include "messages.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cql::protocol {

void FrameHeader::Serialize(std::vector<uint8_t>& buffer) const {
  buffer.push_back(version);
  buffer.push_back(flags);
  buffer.push_back(static_cast<uint8_t>((stream >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(stream & 0xFF));
  buffer.push_back(opcode);
  buffer.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
  buffer.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
  buffer.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(length & 0xFF));
}

void FrameHeader::Deserialize(const uint8_t* data) {
  version = data[0];
  flags = data[1];
  stream = (static_cast<int16_t>(data[2]) << 8) | data[3];
  opcode = data[4];
  length = (static_cast<int32_t>(data[5]) << 24) |
           (static_cast<int32_t>(data[6]) << 16) |
           (static_cast<int32_t>(data[7]) << 8) | data[8];
}

void RequestMessage::SerializeFrame(std::vector<uint8_t>& buffer) const {
  // Reserve space for header
  const auto header_pos = buffer.size();
  buffer.resize(header_pos + FrameHeader::kHeaderSize);

  // Serialize body
  const auto body_pos = buffer.size();
  SerializeBody(buffer);

  // Calculate body length
  const auto body_size = buffer.size() - body_pos;

  // Write header
  FrameHeader header;
  header.flags = flags_;
  header.stream = stream_id_;
  header.opcode = static_cast<uint8_t>(GetOpcode());
  header.length = static_cast<int32_t>(body_size);
  auto container = std::vector<uint8_t>(
      buffer.begin() + header_pos,
      buffer.begin() + header_pos + FrameHeader::kHeaderSize);
  header.Serialize(container);
}

StartupMessage::StartupMessage() {
  flags_ = 0;
  stream_id_ = 0;
}

void StartupMessage::Serialize(std::vector<uint8_t>& buffer) const {
  SerializeFrame(buffer);
}

void StartupMessage::SerializeBody(std::vector<uint8_t>& buffer) const {
  // [string map]
  // Write number of strings (2: CQL_VERSION and optionally COMPRESSION)
  const uint16_t count = compression_.empty() ? 1 : 2;
  buffer.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(count & 0xFF));

  // Write CQL_VERSION
  const std::string key = "CQL_VERSION";
  buffer.push_back(static_cast<uint8_t>((key.size() >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(key.size() & 0xFF));
  buffer.insert(buffer.end(), key.begin(), key.end());

  buffer.push_back(static_cast<uint8_t>((cql_version_.size() >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(cql_version_.size() & 0xFF));
  buffer.insert(buffer.end(), cql_version_.begin(), cql_version_.end());

  if (!compression_.empty()) {
    const std::string comp_key = "COMPRESSION";
    buffer.push_back(static_cast<uint8_t>((comp_key.size() >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(comp_key.size() & 0xFF));
    buffer.insert(buffer.end(), comp_key.begin(), comp_key.end());

    buffer.push_back(static_cast<uint8_t>((compression_.size() >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(compression_.size() & 0xFF));
    buffer.insert(buffer.end(), compression_.begin(), compression_.end());
  }
}

QueryMessage::QueryMessage(std::string query) : query_(std::move(query)) {}

void QueryMessage::Serialize(std::vector<uint8_t>& buffer) const {
  SerializeFrame(buffer);
}

void QueryMessage::SerializeBody(std::vector<uint8_t>& buffer) const {
  // [long string]
  buffer.push_back(static_cast<uint8_t>((query_.size() >> 24) & 0xFF));
  buffer.push_back(static_cast<uint8_t>((query_.size() >> 16) & 0xFF));
  buffer.push_back(static_cast<uint8_t>((query_.size() >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(query_.size() & 0xFF));
  buffer.insert(buffer.end(), query_.begin(), query_.end());

  // [consistency]
  buffer.push_back(static_cast<uint8_t>((consistency_ >> 8) & 0xFF));
  buffer.push_back(static_cast<uint8_t>(consistency_ & 0xFF));

  // [flags]
  buffer.push_back(flags_);
}

ResponseMessage::ResponseMessage(FrameHeader header)
    : header_(std::move(header)) {}

void ResultMessage::Deserialize(const uint8_t* data, std::size_t size) {
  if (size < sizeof(int32_t)) {
    throw std::runtime_error("Invalid result message size");
  }

  kind_ =
      static_cast<ResultKind>((static_cast<int32_t>(data[0]) << 24) |
                              (static_cast<int32_t>(data[1]) << 16) |
                              (static_cast<int32_t>(data[2]) << 8) | data[3]);

  // Additional deserialization based on kind_
}

void ErrorMessage::Deserialize(const uint8_t* data, std::size_t size) {
  if (size < sizeof(int32_t)) {
    throw std::runtime_error("Invalid error message size");
  }

  code_ = (static_cast<int32_t>(data[0]) << 24) |
          (static_cast<int32_t>(data[1]) << 16) |
          (static_cast<int32_t>(data[2]) << 8) | data[3];

  const uint16_t msg_len = (static_cast<uint16_t>(data[4]) << 8) | data[5];

  if (size < 6 + msg_len) {
    throw std::runtime_error("Invalid error message size");
  }

  message_.assign(reinterpret_cast<const char*>(data + 6), msg_len);
}

}  // namespace cql::protocol
