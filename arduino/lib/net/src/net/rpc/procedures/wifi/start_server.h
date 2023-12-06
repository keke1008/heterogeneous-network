#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::wifi::start_server {
    class AsyncParameterDeserializer {
        nb::de::Bin<uint8_t> media_id;
        nb::de::Bin<uint16_t> port;

      public:
        struct Result {
            uint8_t media_id;
            uint16_t port;
        };

        Result result() const {
            return Result{
                .media_id = media_id.result(),
                .port = port.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(media_id.deserialize(r));
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
            const node::LocalNodeService &local_node_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            if (!start_success_.has_value()) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
                const auto &params = params_.result();

                auto &port = link_service.get_port(params.media_id);
                auto result = port->start_wifi_server(params.port);
                if (result.has_value()) {
                    start_success_ = POLL_MOVE_UNWRAP_OR_RETURN(result.value());
                } else {
                    ctx_.set_response_property(Result::BadArgument, 0);
                }
            }

            bool success = POLL_UNWRAP_OR_RETURN(start_success_->poll());
            auto result = success ? Result::Success : Result::Failed;
            ctx_.set_response_property(result, 0);
            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::wifi::start_server
