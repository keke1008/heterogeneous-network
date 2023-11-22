#pragma once

#include "./constants.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <net/routing.h>
#include <stdint.h>

namespace net::rpc {
    enum class FrameType : uint8_t {
        Request = 1,
        Response = 2,
    };

    inline constexpr uint8_t FRAME_TYPE_LENGTH = 1;

    using RawProcedure = uint16_t;

    enum class Procedure : RawProcedure {
        // Debug 1~99
        Blink = 1,

        // Media 100~199
        GetMediaList = 100,

        // Uhf 200~299

        // Wifi 300~399
        ScanAccessPoints = 300,
        GetConnectedAccessPoint = 301,
        ConnectToAccessPoint = 302,
        DisconnectFromAccessPoint = 303,
        StartServer = 320,

        // Serial 400~499

        // Neighbor 500~599
        SendHello = 500,
        SendGoodbye = 501,
        GetNeighborList = 520,
    };

    inline constexpr uint8_t PROCEDURE_LENGTH = 2;

    enum class Result : uint8_t {
        Success = 1,
        Failed = 2,
        NotSupported = 3,
        BadArgument = 4,
        Busy = 5,
        NotImplemented = 6,
        Timeout = 7,
        Unreachable = 8,
    };

    inline constexpr uint8_t RESULT_LENGTH = 1;

    struct RequestHeader {
        FrameType type = FrameType::Request;
        RawProcedure procedure;
    };

    class AsyncRequestFrameHeaderParser {
        nb::buf::AsyncBinParsre<RawProcedure> procedure_parser_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            return procedure_parser_.parse(splitter);
        }

        inline RequestHeader result() {
            return RequestHeader{.procedure = procedure_parser_.result()};
        }
    };

    struct ResponseHeader {
        FrameType type = FrameType::Response;
        RawProcedure procedure;
        Result result;

        static constexpr uint8_t SERIALIZED_LENGTH =
            FRAME_TYPE_LENGTH + PROCEDURE_LENGTH + RESULT_LENGTH;

        void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(nb::buf::FormatBinary(static_cast<uint8_t>(type)));
            builder.append(nb::buf::FormatBinary(static_cast<RawProcedure>(procedure)));
            builder.append(nb::buf::FormatBinary(static_cast<uint8_t>(result)));
        }
    };

    struct RequestInfo {
        RawProcedure procedure;
        routing::NodeId client_id;
        frame::FrameBufferReader body;
    };

    class DeserializeFrame {
        routing::RoutingFrame frame_;
        nb::buf::AsyncOneByteEnumParser<FrameType> frame_type_;
        AsyncRequestFrameHeaderParser header_;

      public:
        explicit DeserializeFrame(routing::RoutingFrame frame) : frame_{etl::move(frame)} {}

        nb::Poll<etl::optional<RequestInfo>> execute() {
            if (frame_.payload.frame_length() < FRAME_TYPE_LENGTH + PROCEDURE_LENGTH) {
                return etl::optional<RequestInfo>{etl::nullopt};
            }

            POLL_UNWRAP_OR_RETURN(frame_.payload.read(frame_type_));
            if (frame_type_.result() != FrameType::Request) {
                return etl::optional<RequestInfo>{etl::nullopt};
            }

            POLL_UNWRAP_OR_RETURN(frame_.payload.read(header_));
            const auto &header = header_.result();

            return etl::optional(RequestInfo{
                .procedure = header.procedure,
                .client_id = frame_.header.sender_id,
                .body = frame_.payload.subreader(),
            });
        }
    };
} // namespace net::rpc
