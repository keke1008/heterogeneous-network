#pragma once

#include "../../request.h"
#include <nb/serde.h>
#include <net/link.h>

namespace net::rpc::dummy::error {
    class Executor {
        RequestContext ctx_;

      public:
        explicit Executor(RequestContext &&ctx, Result result) : ctx_{etl::move(ctx)} {
            ctx_.set_response_property(result, 0);
        }

        inline nb::Poll<void> execute(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::dummy::error
