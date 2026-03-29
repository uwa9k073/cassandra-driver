#include <cassandra/io/buffer_io_base.hpp>
#include <cassandra/io/buffer_reader.hpp>
#include <cassandra/io/cassandra_types.hpp>
#include <cassandra/io/map_types.hpp>
#include <cassandra/io/protocol/frame.hpp>
#include <cassandra/io/protocol/lz4_utils.hpp>
#include <cassandra/io/protocol/message.hpp>
#include <cassandra/io/protocol/types.hpp>
#include <cassandra/result_set.hpp>
#include <io/protocol/column_option.hpp>
#include <io/protocol/events/schema_change.hpp>
#include <memory>
#include <userver/logging/log.hpp>
#include <utility>
#include <variant>
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

    void ParseBody(RawBufferView buffer, Lz4Compressor* compressor = nullptr) {
        if (compressor) {
            auto uncompressed_buffer = compressor->Decompress(buffer);
            DoParseBody(uncompressed_buffer);
        } else {
            DoParseBody(buffer);
        }
    }

    virtual void DoParseBody(RawBufferView data_buffer) = 0;
};

class ErrorMessage : public ResponseMessage {
public:
    ErrorMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    ErrorCode GetErrorCode() const { return error_code; }
    String GetErrorMessage() const { return error_message; }

    void DoParseBody(RawBufferView buffer) override {
        auto reader = BufferReader<BufferView>{buffer};
        error_code = static_cast<ErrorCode>(reader.Read<Int>());
        error_message = reader.Read<String>();
    }

private:
    ErrorCode error_code;
    String error_message;
};

class ReadyMessage : public ResponseMessage {
public:
    ReadyMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    // Ready message does not have a body
    void DoParseBody(RawBufferView /*buffer*/) override {}
};

class AuthentificateMessage : public ResponseMessage {
public:
    AuthentificateMessage(FrameHeader&& header)
        : ResponseMessage(std::move(header)){};

    void DoParseBody(RawBufferView buffer) override {
        auth_challenge = BufferReader<BufferView>{buffer}.Read<String>();
    }

private:
    String auth_challenge;
};

class SupportMessage : public ResponseMessage {
public:
    SupportMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    StringMultiMap GetOptions() const { return options; }

    void DoParseBody(RawBufferView buffer) override {
        options = BufferReader<BufferView>{buffer}.Read<StringMultiMap>();
    }

private:
    StringMultiMap options;
};

class ResultMessage : public ResponseMessage {
    struct ResultColumn {
        String key_space;
        String table;
        String name;
        ColumnOption type;

        static ResultColumn Parse(
            BufferReader<BufferView>& reader, bool global_table_spec_enabled
        ) {
            ResultColumn column;
            if (!global_table_spec_enabled) {
                column.key_space = reader.Read<String>();
                column.table = reader.Read<String>();
            }
            column.name = reader.Read<String>();
            column.type.Parse(reader);
            return column;
        }
    };
    struct RowKind {
        Int flags;
        Int columns_count;
        Bytes paging_state;
        String global_key_space;
        String global_table;
        std::vector<ResultColumn> columns;
        Int rows_count;
        std::vector<BytesBuffer> rows_content;
    };

    struct PreparedKind {
        ShortBytes id;
        Int flags;
        Int column_count;
        Int partition_key_count;
        std::vector<Short> partition_key_indexes;
        String global_key_space;
        String global_table;
        std::vector<ResultColumn> columns;

        static PreparedKind Parse(BufferReader<BufferView>& reader) {
            PreparedKind kind;
            kind.id = reader.Read<ShortBytes>();
            kind.flags = reader.Read<Int>();
            kind.column_count = reader.Read<Int>();
            kind.partition_key_indexes = reader.Read<std::vector<Short>>();
            kind.partition_key_count = kind.partition_key_indexes.size();
            if (kind.flags & static_cast<Int>(Flags::kGlobalTableSpec)) {
                kind.global_key_space = reader.Read<String>();
                kind.global_table = reader.Read<String>();
            }
            for (Int i = 0; i < kind.column_count; ++i) {
                kind.columns.emplace_back(ResultColumn::Parse(
                    reader, kind.flags & static_cast<Int>(Flags::kGlobalTableSpec)
                ));
            }
            return kind;
        }
    };

    struct SetKeyspaceKind {
        String key_space;
    };

    enum class Flags : Int {
        kGlobalTableSpec = 0x0001,
        kHasMorePages = 0x0002,
        kNoMetadata = 0x0004
    };

    void ParseRows(BufferReader<BufferView>& reader) {
        RowKind row_kind;
        row_kind.flags = reader.Read<Int>();
        row_kind.columns_count = reader.Read<Int>();

        if (row_kind.flags & static_cast<Int>(Flags::kHasMorePages)) {
            LOG_DEBUG("HAS_MORE_PAGES_IN_RESULT_MESSAGE");
            row_kind.paging_state = reader.Read<Bytes>();
        }
        if (!(row_kind.flags & static_cast<Int>(Flags::kNoMetadata))) {
            LOG_DEBUG("HAS_METADATA_IN_RESULT_MESSAGE");
            if (row_kind.flags & static_cast<Int>(Flags::kGlobalTableSpec)) {
                row_kind.global_key_space = reader.Read<String>();
                row_kind.global_table = reader.Read<String>();
            }

            row_kind.columns.reserve(row_kind.columns_count);
            for (Int i = 0; i < row_kind.columns_count; ++i) {
                row_kind.columns.emplace_back(ResultColumn::Parse(
                    reader,
                    row_kind.flags & static_cast<Int>(Flags::kGlobalTableSpec)
                ));
            }
        }

        row_kind.rows_count = reader.Read<Int>();
        row_kind.rows_content.reserve(row_kind.rows_count);
        LOG_DEBUG("REMAINING: {}", reader.Remaining());

        for (Int i = 0; i < row_kind.rows_count; ++i) {
            BytesBuffer row_buffer;
            for (int j = 0; j < row_kind.columns_count; ++j) {
                row_buffer.emplace_back(reader.Read<Bytes>());
            }
            row_kind.rows_content.emplace_back(std::move(row_buffer));
        }
        _payload = std::move(row_kind);
    }

    void ParseSetKeyspace(BufferReader<BufferView>& reader) {
        SetKeyspaceKind set_keyspace;
        set_keyspace.key_space = reader.Read<String>();
        _payload = set_keyspace;
    }

public:
    ResultMessage(FrameHeader&& header) : ResponseMessage(std::move(header)){};

    void DoParseBody(RawBufferView buffer) override {
        auto reader = BufferReader<BufferView>{buffer};
        _kind = static_cast<ResultKind>(reader.Read<Int>());

        switch (_kind) {
            case ResultKind::kRows:
                ParseRows(reader);
                break;
            case ResultKind::kSetKeyspace:
                ParseSetKeyspace(reader);
                break;
            case ResultKind::kPrepared:
                _payload = PreparedKind::Parse(reader);
                break;
            case ResultKind::kSchemaChange:
                _payload = events::SchemaChange::Parse(reader);
                break;
            case ResultKind::kVoid:
                _payload = std::monostate{};
                break;
        }
    }

    ResultSet GetResultSet() {
        if (_kind != ResultKind::kRows) {
            LOG_DEBUG("return empty result set");
            return ResultSet{nullptr};
        }
        auto& row_kind = std::get<RowKind>(_payload);
        LOG_DEBUG("GET ROW KIND");
        LOG_DEBUG("RETURN RESULT SET");
        return ResultSet{std::make_shared<cassandra::detail::ResultWrapper>(
            std::move(row_kind.rows_content),
            row_kind.columns_count,
            row_kind.rows_count
        )};
    }

    io::ShortBytes GetPreparedStatementId() {
        if (_kind != ResultKind::kPrepared) {
            LOG_DEBUG("Message is not a prepared result");
            return {};
        }
        auto& prepared_kind = std::get<PreparedKind>(_payload);
        return prepared_kind.id;
    }

private:
    ResultKind _kind;

    using VoidKind = std::monostate;

    using Payload = std::variant<
        VoidKind,
        RowKind,
        SetKeyspaceKind,
        PreparedKind,
        events::SchemaChange>;

    Payload _payload;
};
}  // namespace cassandra::io::protocol
