#pragma once

#include <string>
#include <vector>

#include <cassandra/protocol/frame.hpp>

namespace cassandra::protocol {

/// Base class for all CQL protocol messages
class Message {
public:
    virtual ~Message() = default;
    virtual Opcode GetOpcode() const = 0;
    virtual void Serialize(std::vector<uint8_t>& buffer) const = 0;
    virtual void Deserialize(const uint8_t* data, std::size_t size) = 0;
};

/// Base class for request messages
class RequestMessage : public Message {
public:
    void SerializeFrame(std::vector<uint8_t>& buffer) const;

protected:
    virtual void SerializeBody(std::vector<uint8_t>& buffer) const = 0;
    int16_t stream_id_{0};
    uint8_t flags_{0};
};

/// Base class for response messages
class ResponseMessage : public Message {
public:
    explicit ResponseMessage(FrameHeader header);
    const FrameHeader& GetHeader() const { return header_; }

protected:
    FrameHeader header_;
};

/// STARTUP message
class StartupMessage : public RequestMessage {
public:
    StartupMessage();
    Opcode GetOpcode() const override { return Opcode::kStartup; }
    void Serialize(std::vector<uint8_t>& buffer) const override;
    void Deserialize(const uint8_t* data, std::size_t size) override;

protected:
    void SerializeBody(std::vector<uint8_t>& buffer) const override;

private:
    std::string cql_version_{"3.0.0"};
    std::string compression_;
};

/// READY response
class ReadyMessage : public ResponseMessage {
public:
    using ResponseMessage::ResponseMessage;
    Opcode GetOpcode() const override { return Opcode::kReady; }
    void Serialize(std::vector<uint8_t>& buffer) const override;
    void Deserialize(const uint8_t* data, std::size_t size) override;
};

/// ERROR response
class ErrorMessage : public ResponseMessage {
public:
    using ResponseMessage::ResponseMessage;
    Opcode GetOpcode() const override { return Opcode::kError; }
    void Serialize(std::vector<uint8_t>& buffer) const override;
    void Deserialize(const uint8_t* data, std::size_t size) override;

    int32_t GetCode() const { return code_; }
    const std::string& GetMessage() const { return message_; }

private:
    int32_t code_{0};
    std::string message_;
};

/// AUTHENTICATE response
class AuthenticateMessage : public ResponseMessage {
public:
    using ResponseMessage::ResponseMessage;
    Opcode GetOpcode() const override { return Opcode::kAuthenticate; }
    void Serialize(std::vector<uint8_t>& buffer) const override;
    void Deserialize(const uint8_t* data, std::size_t size) override;

    const std::string& GetAuthenticator() const { return authenticator_; }

private:
    std::string authenticator_;
};

/// QUERY message
class QueryMessage : public RequestMessage {
public:
    explicit QueryMessage(std::string query);
    Opcode GetOpcode() const override { return Opcode::kQuery; }
    void Serialize(std::vector<uint8_t>& buffer) const override;
    void Deserialize(const uint8_t* data, std::size_t size) override;

protected:
    void SerializeBody(std::vector<uint8_t>& buffer) const override;

private:
    std::string query_;
    int16_t consistency_{1};  // ONE
    uint8_t flags_{0};
};

/// RESULT response
class ResultMessage : public ResponseMessage {
public:
    using ResponseMessage::ResponseMessage;
    Opcode GetOpcode() const override { return Opcode::kResult; }
    void Serialize(std::vector<uint8_t>& buffer) const override;
    void Deserialize(const uint8_t* data, std::size_t size) override;

    ResultKind GetKind() const { return kind_; }

private:
    ResultKind kind_{ResultKind::kVoid};
    // Additional fields based on kind
};

}  // namespace cassandra::protocol
