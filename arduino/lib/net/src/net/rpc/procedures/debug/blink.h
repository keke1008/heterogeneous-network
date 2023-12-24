#pragma once

#include "../../request.h"
#include <board/blink.h>
#include <nb/serde.h>

namespace net::rpc::debug::blink {
    enum class Operation : uint8_t {
        Blink = 1,
        Stop = 2,
    };

    class AsyncParameterDeserializer {
        nb::de::Bin<uint8_t> operation_;

      public:
        struct Result {
            Operation operation;
        };

        inline etl::optional<Result> result() {
            switch (operation_.result()) {
            case static_cast<uint8_t>(Operation::Blink):
            case static_cast<uint8_t>(Operation::Stop):
                return Result{.operation = static_cast<Operation>(operation_.result())};
            default:
                return etl::nullopt;
            }
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return operation_.deserialize(r);
        }

        inline uint8_t raw_operation() const {
            return operation_.result();
        }
    };

    template <nb::AsyncReadableWritable RW>
    class Executor {
        enum class State {
            Deserialize,
            Respond,
        } state_{State::Deserialize};

        RequestContext<RW> ctx_;
        AsyncParameterDeserializer params_;

      public:
        explicit Executor(RequestContext<RW> &&context) : ctx_{etl::move(context)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            const node::LocalNodeService &local_node_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            if (state_ == State::Deserialize) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
                auto opt_params = params_.result();
                if (!opt_params) {
                    LOG_WARNING(FLASH_STRING("Invalid operation: "), params_.raw_operation());
                    ctx_.set_response_property(Result::BadArgument, 0);
                    state_ = State::Respond;
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                switch (opt_params->operation) {
                case Operation::Blink:
                    LOG_INFO(FLASH_STRING("Blinking"));
                    board::blink::blink();
                    ctx_.set_response_property(Result::Success, 0);
                    break;
                case Operation::Stop:
                    LOG_INFO(FLASH_STRING("Blinking stopped"));
                    board::blink::stop();
                    ctx_.set_response_property(Result::Success, 0);
                    break;
                }

                state_ = State::Respond;
            }

            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::debug::blink
