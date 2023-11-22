#pragma once

#include "../../request.h"
#include <nb/buf.h>

namespace net::rpc::debug {
    enum class BlinkOperation : uint8_t {
        Blink = 1,
        Stop = 2,
    };

    class BlinkExecutor {
        enum class State {
            Deserialize,
            Respond,
        } state_{State::Deserialize};

        RequestContext ctx_;
        nb::buf::AsyncOneByteEnumParser<BlinkOperation> operation_parser_;

      public:
        explicit BlinkExecutor(RequestContext &&context) : ctx_{etl::move(context)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, routing_service, time, rand);
            }

            if (state_ == State::Deserialize) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().read(operation_parser_));
                auto operation = operation_parser_.result();
                switch (operation) {
                case BlinkOperation::Blink:
                    LOG_INFO("Blinking");
                    ctx_.set_response_property(Result::Success, 0);
                case BlinkOperation::Stop:
                    LOG_INFO("Blinking stopped");
                    ctx_.set_response_property(Result::Success, 0);
                default:
                    ctx_.set_response_property(Result::BadArgument, 0);
                }

                state_ = State::Respond;
            }

            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::debug
