#pragma once

#include "./constants.h"
#include "./frame.h"
#include <net/routing.h>

namespace net::rpc {
    class Request {
        Procedure procedure_;
        routing::NodeId client_node_id_;
        frame::FrameBufferReader body_;

      public:
        inline Request(
            Procedure procedure,
            const routing::NodeId &client_node_id,
            frame::FrameBufferReader &&body,
            util::Time &time
        )
            : procedure_{procedure},
              client_node_id_{client_node_id},
              body_{etl::move(body)} {}

        inline Procedure procedure() const {
            return procedure_;
        }

        inline const routing::NodeId &client_node_id() const {
            return client_node_id_;
        }

        inline const frame::FrameBufferReader &body() const {
            return body_;
        }

        inline frame::FrameBufferReader &body() {
            return body_;
        }
    };

    class Response {
        etl::optional<nb::Future<etl::expected<void, net::routing::neighbor::SendError>>> future_;
        etl::optional<Result> result_;
        etl::optional<frame::FrameBufferWriter> response_writer_;

      public:
        inline void set_result(Result result) {
            result_ = result;
        }

        nb::Poll<etl::reference_wrapper<frame::FrameBufferWriter>> poll_response_frame_writer(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            routing::RoutingSocket &socket,
            const routing::NodeId &client_node_id,
            Procedure procedure,
            uint8_t body_length
        ) {
            ASSERT(result_.has_value());

            if (response_writer_.has_value()) {
                return etl::ref(response_writer_.value());
            }

            uint8_t length = body_length + ResponseHeader::SERIALIZED_LENGTH;
            response_writer_ = POLL_MOVE_UNWRAP_OR_RETURN(
                socket.poll_frame_writer(frame_service, routing_service, client_node_id, length)
            );
            response_writer_->write(ResponseHeader{.procedure = procedure, .result = *result_});
            return etl::ref(response_writer_.value());
        }

        inline bool is_response_filled() const {
            return response_writer_.has_value() && response_writer_->is_buffer_filled();
        }

        inline nb::Poll<etl::expected<void, net::routing::neighbor::SendError>> poll_send_response(
            routing::RoutingService &routing_service,
            routing::RoutingSocket &socket,
            util::Time &time,
            util::Rand &rand,
            const routing::NodeId &client_node_id
        ) {
            if (!future_.has_value()) {
                auto &&reader = response_writer_->make_initial_reader();
                future_ = POLL_MOVE_UNWRAP_OR_RETURN(socket.poll_send_frame(
                    routing_service, client_node_id, etl::move(reader), time, rand
                ));
            }

            const auto &result = POLL_UNWRAP_OR_RETURN(future_->poll());
            return result.get();
        }
    };

    class RequestContext {
        memory::Static<routing::RoutingSocket> &socket_;
        Request request_;
        Response response_{};
        nb::Delay response_timeout_;

      public:
        explicit RequestContext(
            util::Time &time,
            memory::Static<routing::RoutingSocket> &socket,
            Request &&request
        )
            : socket_{socket},
              request_{etl::move(request)},
              response_timeout_{time, RESPONSE_TIMEOUT} {}

        inline Request &request() {
            return request_;
        }

        inline void set_result(Result result) {
            response_.set_result(result);
        }

        inline nb::Poll<etl::reference_wrapper<frame::FrameBufferWriter>>
        poll_response_frame_writer(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            uint8_t body_length
        ) {
            return response_.poll_response_frame_writer(
                frame_service, routing_service, socket_.get(), request_.client_node_id(),
                request_.procedure(), body_length
            );
        }

        inline bool is_response_filled() const {
            return response_.is_response_filled();
        }

        inline nb::Poll<etl::expected<void, net::routing::neighbor::SendError>> poll_send_response(
            routing::RoutingService &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            return response_.poll_send_response(
                routing_service, socket_.get(), time, rand, request_.client_node_id()
            );
        }

        inline bool is_timeout(util::Time &time) {
            return response_timeout_.poll(time).is_ready();
        }
    };

    class RequestReceiver {
        memory::Static<routing::RoutingSocket> socket_;
        etl::optional<DeserializeFrame> deserializer_;

      public:
        explicit RequestReceiver(routing::RoutingSocket &&socket) : socket_{etl::move(socket)} {}

        inline etl::optional<RequestContext> execute(util::Time &time) {
            if (!deserializer_.has_value()) {
                auto &&poll_frame = socket_->poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return etl::nullopt;
                }
                deserializer_.emplace(etl::move(poll_frame.unwrap()));
            }

            auto &&poll_opt_request_info = deserializer_->execute();
            if (poll_opt_request_info.is_pending() || !poll_opt_request_info.unwrap().has_value()) {
                return etl::nullopt;
            }

            auto &&request_info = poll_opt_request_info.unwrap().value();
            return RequestContext{
                time,
                socket_,
                Request{
                    request_info.procedure,
                    request_info.client_id,
                    etl::move(request_info.body),
                    time,
                },
            };
        }
    };
} // namespace net::rpc