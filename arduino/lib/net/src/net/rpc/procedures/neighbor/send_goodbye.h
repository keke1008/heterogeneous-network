#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::neighbor::send_goodbye {
    class AsyncParameterDeserializer {
        routing::AsyncNodeIdDeserializer node_id_;

      public:
        struct Result {
            routing::NodeId node_id;
        };

        inline Result result() {
            return Result{.node_id = node_id_.result()};
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return node_id_.deserialize(r);
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer params_;

      public:
        explicit Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            link::LinkService &link_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, routing_service, time, rand);
            }

            POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
            const auto &params = params_.result();

            POLL_UNWRAP_OR_RETURN(routing_service.poll_send_goodbye(params.node_id));
            ctx_.set_response_property(Result::Success, 0);

            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::neighbor::send_goodbye
