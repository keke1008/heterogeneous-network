#pragma once

#include <etl/optional.h>
#include <log.h>
#include <net/frame/service.h>
#include <net/routing/service.h>
#include <net/routing/socket.h>
#include <stdint.h>

namespace net::rpc {
    enum class FrameType : uint8_t {
        Request = 1,
        Response = 2,
    };

    enum class Procedure : uint8_t {
        RoutingNeighborSendHello = 1,
        RoutingNeighborSendGoodbye = 2,
        LinkGetMediaList = 3,
        LinkWifiScanAp = 4,
        LinkWifiConnectAp = 5,
    };

    enum class Result : uint8_t {
        Success = 1,
        Failed = 2,
        NotSupported = 3,
        BadArgument = 4,
        Busy = 5,
        NotImplemented = 6,
    };

    struct RequestFrameHeader {
        routing::NodeId client_node_id;
        routing::NodeId server_node_id;
        Procedure procedure;
    };

    class AsyncRequestFrameHeaderParser {
        nb::buf::AsyncOneByteEnumParser<FrameType> frame_type_parser_;
        routing::AsyncNodeIdParser client_node_id_parser_;
        routing::AsyncNodeIdParser server_node_id_parser_;
        nb::buf::AsyncOneByteEnumParser<Procedure> procedure_parser_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            POLL_UNWRAP_OR_RETURN(frame_type_parser_.parse(splitter));
            POLL_UNWRAP_OR_RETURN(client_node_id_parser_.parse(splitter));
            POLL_UNWRAP_OR_RETURN(server_node_id_parser_.parse(splitter));
            return procedure_parser_.parse(splitter);
        }

        inline RequestFrameHeader result() {
            return RequestFrameHeader{
                .client_node_id = client_node_id_parser_.result(),
                .server_node_id = server_node_id_parser_.result(),
                .procedure = procedure_parser_.result(),
            };
        }
    };

    template <frame::IFrameService FrameService>
    class Request {
        Procedure procedure_;
        frame::FrameBufferReader body_;
        routing::Socket<FrameService> socket_;
        nb::Delay response_timeout_;

      public:
        inline Request(
            Procedure procedure,
            frame::FrameBufferReader &&body,
            routing::Socket<FrameService> &&socket,
            util::Time &time
        )
            : procedure_{procedure},
              body_{etl::move(body)},
              socket_{etl::move(socket)},
              response_timeout_{time, util::Duration::from_seconds(5)} {}

        Procedure procedure() const {
            return procedure_;
        }

        inline const frame::FrameBufferReader &body() const {
            return body_;
        }

        inline frame::FrameBufferReader &body() {
            return body_;
        }

        inline bool is_timeout(util::Time &time) {
            return response_timeout_.poll(time).is_ready();
        }

        nb::Poll<frame::FrameBufferWriter>
        request_response_frame_writer(uint8_t body_length, Result result) {
            ASSERT(body_length <= 255 - 3, "overflow detected");
            uint8_t length = body_length + 3;
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(socket_.request_frame_writer(length));
            writer.write(static_cast<uint8_t>(FrameType::Response));
            writer.write(static_cast<uint8_t>(procedure_));
            writer.write(static_cast<uint8_t>(result));
            return etl::move(writer);
        }

        inline nb::Poll<void> send_response(frame::FrameBufferReader &&reader) {
            POLL_UNWRAP_OR_RETURN(socket_.send_frame(etl::move(reader)));
            return nb::ready();
        }
    };

    template <frame::IFrameService FrameService>
    class RequestReceiver {
        routing::Socket<FrameService> socket_;
        etl::optional<etl::pair<routing::Frame, AsyncRequestFrameHeaderParser>> parser_;

      public:
        explicit RequestReceiver(routing::Socket<FrameService> &&socket)
            : socket_{etl::move(socket)} {}

        inline nb::Poll<Request<FrameService>> execute(util::Time &time) {
            if (!parser_.has_value()) {
                auto frame = POLL_MOVE_UNWRAP_OR_RETURN(socket_.receive_frame());
                parser_.emplace(etl::move(frame), AsyncRequestFrameHeaderParser{});
            }

            auto &[frame, parser] = parser_.value();
            POLL_UNWRAP_OR_RETURN(frame.reader.read(parser));
            const auto &header = parser.result();
            return Request<FrameService>{
                etl::move(socket_),
                header.procedure,
                etl::move(frame.reader),
                time,
            };
        }
    };

    template <frame::IFrameService FrameService>
    class ResponseHeaderWriter {
        uint8_t length_;
        Result result_;
        etl::optional<frame::FrameBufferWriter> writer_;

      public:
        explicit ResponseHeaderWriter(uint8_t length, Result result)
            : length_{length},
              result_{result} {}

        inline nb::Poll<etl::reference_wrapper<frame::FrameBufferWriter>>
        execute(Request<FrameService> &request) {
            if (!writer_.has_value()) {
                writer_ = POLL_MOVE_UNWRAP_OR_RETURN(
                    request.request_response_frame_writer(length_, result_)
                );
            }
            return etl::ref(writer_.value());
        }
    };
} // namespace net::rpc
