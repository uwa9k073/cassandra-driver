#include <cassandra/detail/query_parameters.hpp>
#include <cassandra/io/buffer_writer.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/options.hpp>
#include <cstdint>
#include <unordered_map>
#include <userver/logging/log.hpp>
#include <variant>
#include <vector>
#include "cassandra/batch_query.hpp"

namespace cassandra::io::protocol {
class RequestMessage : public Message {
public:
    RequestMessage(FrameHeader&& frame) : Message(std::move(frame)) {}

    void Serialize(RawBuffer& buffer, Compressor* compressor_ptr = nullptr) {
        if (!compressor_ptr) {
            // Serialize the message header
            _header.Serialize(buffer);
            // Serialize the message body
            SerializeBody(buffer);
        } else {
            _header.flags |= static_cast<uint8_t>(Flags::kCompression);

            // Serialize the message header
            _header.Serialize(buffer);

            RawBuffer body_buffer;
            // Serialize the message body into a separate buffer
            SerializeBody(body_buffer);
            // Compress the body
            compressor_ptr->Compress(body_buffer, buffer);
        }
        // write body length if length is not null
        if (int32_t body_length = buffer.size() - FrameHeader::kHeaderSize;
            body_length > 0) {
            _header.UpdateLength(buffer, body_length);
        }
    }
    // we only need to implement SerializeBody in derived classes,
    // and it should append to the buffer and rewrite the header
    // length if necessary
    virtual void SerializeBody(RawBuffer& buffer) = 0;
};

class OptionsMessage final : public RequestMessage {
public:
    OptionsMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kOptions}) {}
    void SerializeBody(RawBuffer& /*buffer*/) override {}
};

class StartupMessage final : public RequestMessage {
public:
    StartupMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kStartup}) {
        options[String("CQL_VERSION")] = String("3.0.0");
    }
    StartupMessage(std::string_view compression_protocol)
        : RequestMessage(FrameHeader{.opcode = Opcode::kStartup}) {
        options[String("CQL_VERSION")] = String("3.0.0");
        options[String("COMPRESSION")] =
            String(compression_protocol.data(), compression_protocol.size());
    }

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(options);
    }

private:
    StringMap options;
};

enum class QueryFlags : Byte {
    kValues = 0x01,
    kSkipMetadata = 0x02,
    kPageSize = 0x04,
    kPagingState = 0x08,
    kSerialConsistency = 0x10,
    kTimestamp = 0x20,
    kNamesForValues = 0x40,
};

class QueryMessage final : public RequestMessage {
public:
    QueryMessage() : RequestMessage(FrameHeader{.opcode = Opcode::kQuery}) {}
    QueryMessage(Consistency level, const LongString& query)
        : RequestMessage(FrameHeader{.opcode = Opcode::kQuery}),
          consistency_level(level),
          query(query) {}
    explicit QueryMessage(
        Consistency level, const LongString& query, const QueryParameters& params
    )
        : RequestMessage(FrameHeader{.opcode = Opcode::kQuery}),
          consistency_level(level),
          query(query),
          params(params) {}

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(query);
        writer.Write(static_cast<Short>(consistency_level));
        Byte flags = static_cast<Byte>(QueryFlags::kSkipMetadata);
        if (!params.Empty()) {
            flags |= 0x01;
        }
        writer.Write(flags);
        if (!params.Empty()) {
            LOG_DEBUG("PARAM COUNT: {}", params.Size());
            writer.Write<Short>(params.Size());
            auto* buffers = params.ParamBuffers();
            for (std::size_t i = 0; i < params.Size(); ++i) {
                writer.Write<io::Bytes>(buffers[i]);
            }
        }
    }

private:
    Consistency consistency_level;
    LongString query;
    QueryParameters params;
};

class PrepareMessage : public RequestMessage {
public:
    PrepareMessage(const LongString& query)
        : RequestMessage(FrameHeader{.opcode = Opcode::kPrepare}), query(query) {}

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(query);
    }

private:
    LongString query;
};

class ExecuteMessage : public RequestMessage {
public:
    ExecuteMessage(
        Consistency consistency_level, const ShortBytes& id, QueryParameters params
    )
        : RequestMessage(FrameHeader{.opcode = Opcode::kExecute}),
          _id(id),
          _consistency_level(consistency_level),
          _params(params) {}

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(_id);
        writer.Write(static_cast<Short>(_consistency_level));
        Byte flags = static_cast<Byte>(QueryFlags::kSkipMetadata);
        if (!_params.Empty()) {
            flags |= 0x01;
        }
        writer.Write(flags);
        if (!_params.Empty()) {
            LOG_DEBUG("PARAM COUNT: {}", _params.Size());
            writer.Write<Short>(_params.Size());
            auto* buffers = _params.ParamBuffers();
            for (std::size_t i = 0; i < _params.Size(); ++i) {
                writer.Write<io::Bytes>(buffers[i]);
            }
        }
    }

private:
    const ShortBytes& _id;
    Consistency _consistency_level;
    QueryParameters _params;
};

class BatchMessage final : public RequestMessage {
public:
    BatchMessage(
        const std::vector<BatchStatement>& queries, Consistency consistency_level
    )
        : RequestMessage(FrameHeader{.opcode = Opcode::kBatch}),
          _queries(queries),
          _consistency_level(consistency_level) {}

    void SerializeBody(RawBuffer& buffer) override {
        BufferWriter writer(buffer);
        writer.Write(_type);
        LOG_DEBUG("WRITING QUERIES");
        writer.Write<Short>(_queries.size());
        for (const auto& batch_query : _queries) {
            writer.Write<Byte>(static_cast<Byte>(batch_query.kind));
            if (batch_query.kind == BatchStatement::Kind::kString) {
                LOG_DEBUG("WRITING STRING QUERY");
                writer.Write<LongString>(std::get<LongString>(batch_query.query));
            } else {
                LOG_DEBUG("WRITING PREPARED QUERY");
                writer.Write<ShortBytes>(std::get<ShortBytes>(batch_query.query));
            }
            LOG_DEBUG(
                "WRITING BATCH STATEMENT QUERY PARAMS, size={}",
                batch_query.params.Size()
            );
            writer.Write<Short>(batch_query.params.Size());
            if (!batch_query.params.Empty()) {
                const auto* buffers = batch_query.params.ParamBuffers();
                for (std::size_t i = 0; i < batch_query.params.Size(); ++i) {
                    LOG_DEBUG("WRITING PARAM: {}", i);
                    writer.Write<io::Bytes>(buffers[i]);
                }
            }
        }

        writer.Write<Short>(static_cast<Short>(_consistency_level));
        writer.Write<Byte>(flags);
    }

private:
    Byte _type = 0;
    std::vector<BatchStatement> _queries;
    Consistency _consistency_level;
    Byte flags = 0;
};

}  // namespace cassandra::io::protocol
