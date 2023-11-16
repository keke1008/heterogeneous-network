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

    enum class Procedure : uint16_t {
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
        Procedure procedure;
    };

    class AsyncRequestFrameHeaderParser {
        nb::buf::AsyncBinParsre<uint16_t> procedure_parser_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            return procedure_parser_.parse(splitter);
        }

        inline etl::optional<RequestHeader> result() {
            uint16_t procedure = procedure_parser_.result();
            switch (static_cast<Procedure>(procedure)) {
            case Procedure::Blink:
            case Procedure::GetMediaList:
            case Procedure::ScanAccessPoints:
            case Procedure::ConnectToAccessPoint:
            case Procedure::StartServer:
            case Procedure::SendHello:
            case Procedure::SendGoodbye:
            case Procedure::GetNeighborList:
                return RequestHeader{.procedure = static_cast<Procedure>(procedure)};
            default:
                return etl::nullopt;
            }
        }
    };

    struct ResponseHeader {
        FrameType type = FrameType::Response;
        Procedure procedure;
        Result result;

        static constexpr uint8_t SERIALIZED_LENGTH =
            FRAME_TYPE_LENGTH + PROCEDURE_LENGTH + RESULT_LENGTH;

        void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(nb::buf::FormatBinary(static_cast<uint8_t>(type)));
            builder.append(nb::buf::FormatBinary(static_cast<uint16_t>(procedure)));
            builder.append(nb::buf::FormatBinary(static_cast<uint8_t>(result)));
        }
    };

    struct RequestInfo {
        Procedure procedure;
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
            const auto &opt_header = header_.result();
            if (!opt_header.has_value()) {
                return etl::optional<RequestInfo>{etl::nullopt};
            }

            return etl::optional(RequestInfo{
                .procedure = opt_header->procedure,
                .client_id = frame_.header.sender_id,
                .body = frame_.payload.subreader(),
            });
        }
    };
} // namespace net::rpc
