#pragma once

#include "./request_id.h"
#include <etl/optional.h>
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
        SetAddress = 400,

        // Neighbor 500~599
        SendHello = 500,
        SendGoodbye = 501,
        GetNeighborList = 520,

        // Routing 600~699

        // Address 700~799
        ResolveAddress = 700,

        // VRouter 800~899
        GetVRouters = 800,
        CreateVRouter = 801,
        DeleteVRouter = 802,
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
        BadResponseFormat = 9,
        InvalidOperation = 10,
    };

    inline constexpr uint8_t RESULT_LENGTH = 1;

    struct CommonHeader {
        FrameType type;
        RawProcedure procedure;
        RequestId request_id;
    };

    class AsyncFrameCommonHeaderDeserializer {
        nb::de::Bin<uint8_t> frame_type_;
        nb::de::Bin<uint16_t> procedure_;
        AsyncRequestIdDeserializer request_id_;

      public:
        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(frame_type_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(procedure_.deserialize(r));
            return request_id_.deserialize(r);
        }

        inline CommonHeader result() {
            return CommonHeader{
                .type = static_cast<FrameType>(frame_type_.result()),
                .procedure = procedure_.result(),
                .request_id = request_id_.result(),
            };
        }
    };

    struct ResponseHeader {
        FrameType type = FrameType::Response;
        RawProcedure procedure;
        RequestId request_id;
        Result result;
    };

    class AsyncResponseHeaderSerializer {
        nb::ser::Bin<uint8_t> frame_type_;
        nb::ser::Bin<RawProcedure> procedure_;
        AsyncRequestIdSerializer request_id_;
        nb::ser::Bin<uint8_t> result_;

      public:
        explicit AsyncResponseHeaderSerializer(
            RawProcedure procedure,
            RequestId request_id,
            Result result
        )
            : frame_type_{static_cast<uint8_t>(FrameType::Response)},
              procedure_{procedure},
              request_id_{request_id},
              result_{static_cast<uint8_t>(result)} {}

        template <nb::ser::AsyncWritable W>
        nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(frame_type_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(procedure_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(request_id_.serialize(w));
            return result_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return frame_type_.serialized_length() + procedure_.serialized_length() +
                request_id_.serialized_length() + result_.serialized_length();
        }
    };

    struct RequestInfo {
        RawProcedure procedure;
        RequestId request_id;
        node::Source client;
        frame::FrameBufferReader body;
    };

    class DeserializeFrame {
        routing::RoutingFrame frame_;
        AsyncFrameCommonHeaderDeserializer header_;

      public:
        explicit DeserializeFrame(routing::RoutingFrame frame) : frame_{etl::move(frame)} {}

        nb::Poll<etl::optional<RequestInfo>> execute() {
            if (frame_.payload.buffer_length() < FRAME_TYPE_LENGTH + PROCEDURE_LENGTH) {
                return etl::optional<RequestInfo>{etl::nullopt};
            }

            auto result = POLL_UNWRAP_OR_RETURN(frame_.payload.deserialize(header_));
            if (result != nb::de::DeserializeResult::Ok) {
                return etl::optional<RequestInfo>{etl::nullopt};
            }

            const auto &header = header_.result();
            if (header.type != FrameType::Request) {
                return etl::optional<RequestInfo>{etl::nullopt};
            }

            return etl::optional(RequestInfo{
                .procedure = header.procedure,
                .request_id = header.request_id,
                .client = frame_.source,
                .body = frame_.payload.subreader(),
            });
        }
    };
} // namespace net::rpc
