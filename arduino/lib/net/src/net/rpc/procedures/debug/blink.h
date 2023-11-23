#pragma once

#include "../../request.h"
#include <board/blink.h>
#include <nb/serde.h>

namespace net::rpc::debug::blink {
    enum class Operation : uint8_t {
        Blink = 1,
        Stop = 2,
        Unknown = 255,
    };

    class AsyncParameterDeserializer {
        nb::de::Bin<uint8_t> operation_;

      public:
        struct Result {
            Operation operation;
        };

        inline Result result() {
            switch (operation_.result()) {
            case static_cast<uint8_t>(Operation::Blink):
            case static_cast<uint8_t>(Operation::Stop):
                return Result{.operation = static_cast<Operation>(operation_.result())};
            default:
                return Result{.operation = Operation::Unknown};
            }
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return operation_.deserialize(r);
        }
    };

    class Executor {
        enum class State {
            Deserialize,
            Respond,
        } state_{State::Deserialize};

        RequestContext ctx_;
        AsyncParameterDeserializer params_;

      public:
        explicit Executor(RequestContext &&context) : ctx_{etl::move(context)} {}

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
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
                auto operation = params_.result().operation;

                switch (operation) {
                case Operation::Blink:
                    LOG_INFO("Blinking");
                    board::blink::blink();
                    ctx_.set_response_property(Result::Success, 0);
                case Operation::Stop:
                    LOG_INFO("Blinking stopped");
                    board::blink::stop();
                    ctx_.set_response_property(Result::Success, 0);
                case Operation::Unknown:
                    LOG_WARNING("Unknown operation");
                    ctx_.set_response_property(Result::BadArgument, 0);
                }

                state_ = State::Respond;
            }

            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::debug::blink
