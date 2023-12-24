#pragma once

#include "./constants.h"
#include "./frame.h"
#include "./request_id.h"
#include <net/routing.h>

namespace net::rpc {
    class Request {
        RawProcedure procedure_;
        RequestId request_id_;
        node::Source client_;
        frame::FrameBufferReader body_;

      public:
        inline Request(
            RawProcedure procedure,
            RequestId request_id,
            const node::Source &client,
            frame::FrameBufferReader &&body,
            util::Time &time
        )
            : procedure_{procedure},
              request_id_{request_id},
              client_{client},
              body_{etl::move(body)} {}

        inline RawProcedure procedure() const {
            return procedure_;
        }

        inline RequestId request_id() const {
            return request_id_;
        }

        inline const node::Source &client() const {
            return client_;
        }

        inline const frame::FrameBufferReader &body() const {
            return body_;
        }

        inline frame::FrameBufferReader &body() {
            return body_;
        }
    };

    struct ResponseProperty {
        Result result;
        uint8_t body_length;
    };

    class Response {
        etl::optional<nb::Future<etl::expected<void, net::neighbor::SendError>>> future_;
        etl::optional<ResponseProperty> property_;
        etl::optional<AsyncResponseHeaderSerializer> header_serializer_;
        etl::optional<frame::FrameBufferWriter> response_writer_;

      public:
        inline void set_property(Result result, uint8_t body_length) {
            if (future_.has_value()) {
                return;
            }
            if (response_writer_.has_value()) {
                response_writer_ = etl::nullopt;
            }
            property_ = ResponseProperty{.result = result, .body_length = body_length};
        }

        template <nb::AsyncReadableWritable RW>
        nb::Poll<etl::reference_wrapper<frame::FrameBufferWriter>> poll_response_frame_writer(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE> &socket,
            util::Rand &rand,
            const Request &request
        ) {
            FASSERT(property_.has_value());

            if (!header_serializer_.has_value()) {
                header_serializer_ = AsyncResponseHeaderSerializer{
                    request.procedure(),
                    request.request_id(),
                    property_->result,
                };
            }

            if (!response_writer_.has_value()) {
                uint8_t length = property_->body_length + header_serializer_->serialized_length();
                response_writer_ = POLL_MOVE_UNWRAP_OR_RETURN(socket.poll_frame_writer(
                    fs, lns, rand, node::Destination(request.client()), length
                ));
                response_writer_->serialize_all_at_once(*header_serializer_);
            }

            return etl::ref(response_writer_.value());
        }

        inline bool is_response_property_set() const {
            return property_.has_value();
        }

        inline bool is_ready_to_send_response() const {
            return response_writer_.has_value() && response_writer_->is_all_written();
        }

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<etl::expected<void, net::neighbor::SendError>> poll_send_response(
            const local::LocalNodeService &lns,
            routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE> &socket,
            util::Time &time,
            util::Rand &rand,
            const node::Source &client
        ) {
            if (!future_.has_value()) {
                FASSERT(is_ready_to_send_response());

                auto &&reader = response_writer_->create_reader();
                future_ = POLL_MOVE_UNWRAP_OR_RETURN(
                    socket.poll_send_frame(lns, node::Destination(client), etl::move(reader))
                );
            }

            const auto &result = POLL_UNWRAP_OR_RETURN(future_->poll());
            return result.get();
        }
    };

    template <nb::AsyncReadableWritable RW>
    class RequestContext {
        memory::Static<routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE>> &socket_;
        Request request_;
        Response response_{};
        nb::Delay response_timeout_;

      public:
        explicit RequestContext(
            util::Time &time,
            memory::Static<routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE>> &socket,
            Request &&request
        )
            : socket_{socket},
              request_{etl::move(request)},
              response_timeout_{time, RESPONSE_TIMEOUT} {}

        inline Request &request() {
            return request_;
        }

        inline void set_timeout_duration(util::Duration duration) {
            response_timeout_.set_duration(duration);
        }

        inline void set_response_property(Result result, uint8_t body_length) {
            response_.set_property(result, body_length);
        }

        inline nb::Poll<etl::reference_wrapper<frame::FrameBufferWriter>> poll_response_writer(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            util::Rand &rand
        ) {
            return response_.poll_response_frame_writer(fs, lns, socket_.get(), rand, request_);
        }

        inline bool is_response_property_set() const {
            return response_.is_response_property_set();
        }

        inline bool is_ready_to_send_response() const {
            return response_.is_ready_to_send_response();
        }

        nb::Poll<void> poll_send_response(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            util::Time &time,
            util::Rand &rand
        ) {
            if (response_timeout_.poll(time).is_ready()) {
                set_response_property(Result::Timeout, 0);
            }
            if (is_response_property_set()) {
                POLL_UNWRAP_OR_RETURN(poll_response_writer(fs, lns, rand));
            }
            if (is_ready_to_send_response()) {
                POLL_UNWRAP_OR_RETURN(
                    response_.poll_send_response(lns, socket_.get(), time, rand, request_.client())
                );
                return nb::ready();
            }
            return nb::pending;
        }
    };

    template <nb::AsyncReadableWritable RW>
    class RequestReceiver {
        memory::Static<routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE>> socket_;
        etl::optional<DeserializeFrame> deserializer_;

      public:
        explicit RequestReceiver(routing::RoutingSocket<RW, FRAME_ID_CACHE_SIZE> &&socket)
            : socket_{etl::move(socket)} {}

        inline void execute(
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_->execute(lns, ns, ds, time, rand);
        }

        inline etl::optional<RequestContext<RW>> poll_receive_frame(util::Time &time) {
            if (!deserializer_.has_value()) {
                auto &&poll_frame = socket_->poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return etl::nullopt;
                }
                deserializer_.emplace(etl::move(poll_frame.unwrap()));
            }

            auto &&poll_opt_request_info = deserializer_->execute();
            if (poll_opt_request_info.is_pending()) { // まだフレームを受信していない
                return etl::nullopt;
            }

            deserializer_.reset();
            if (!poll_opt_request_info.unwrap().has_value()) { // フレームのデシリアライズに失敗
                return etl::nullopt;
            }

            auto &&request_info = poll_opt_request_info.unwrap().value();
            return RequestContext{
                time,
                socket_,
                Request{
                    request_info.procedure,
                    request_info.request_id,
                    request_info.client,
                    etl::move(request_info.body),
                    time,
                },
            };
        }
    };
} // namespace net::rpc
