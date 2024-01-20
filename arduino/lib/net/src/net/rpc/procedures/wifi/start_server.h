#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::wifi::start_server {
    class AsyncParameterDeserializer {
        link::AsyncMediaPortNumberDeserializer media_number_;
        nb::de::Bin<uint16_t> port;

      public:
        struct Result {
            link::MediaPortNumber media_number;
            uint16_t port;
        };

        Result result() const {
            return Result{
                .media_number = media_number_.result(),
                .port = port.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(media_number_.deserialize(r));
            return port.deserialize(r);
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer params_;
        etl::optional<nb::Future<bool>> start_success_;

      public:
        explicit Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            link::MediaService auto &ms,
            const net::local::LocalNodeService &local_node_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            if (!start_success_.has_value()) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
                const auto &params = params_.result();

                auto opt_ref_port = ms.get_media_port(params.media_number);
                if (!opt_ref_port.has_value()) {
                    ctx_.set_response_property(Result::InvalidOperation, 0);
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                auto &port = opt_ref_port->get();
                etl::expected<nb::Poll<nb::Future<bool>>, link::MediaPortUnsupportedOperation> res =
                    port.wifi_start_server(params.port, time);
                if (!res.has_value()) {
                    ctx_.set_response_property(Result::BadArgument, 0);
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                start_success_ = POLL_MOVE_UNWRAP_OR_RETURN(res.value());
            }

            bool success = POLL_UNWRAP_OR_RETURN(start_success_->poll());
            auto result = success ? Result::Success : Result::Failed;
            ctx_.set_response_property(result, 0);
            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::wifi::start_server
