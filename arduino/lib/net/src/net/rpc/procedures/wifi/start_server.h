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

    template <nb::AsyncReadableWritable RW>
    class Executor {
        RequestContext<RW> ctx_;
        AsyncParameterDeserializer params_;
        etl::optional<nb::Future<bool>> start_success_;

      public:
        explicit Executor(RequestContext<RW> &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
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

                auto opt_ref_port = link_service.get_port(params.media_number);
                if (!opt_ref_port.has_value()) {
                    ctx_.set_response_property(Result::InvalidOperation, 0);
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                auto opt_ref_wifi = opt_ref_port->get()->get_wifi_interactor();
                if (!opt_ref_wifi.has_value()) {
                    ctx_.set_response_property(Result::InvalidOperation, 0);
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                auto &wifi = opt_ref_wifi->get();
                start_success_ = POLL_MOVE_UNWRAP_OR_RETURN(wifi.start_server(params.port, time));
            }

            bool success = POLL_UNWRAP_OR_RETURN(start_success_->poll());
            auto result = success ? Result::Success : Result::Failed;
            ctx_.set_response_property(result, 0);
            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::wifi::start_server
