#pragma once

#include "../../request.h"
#include <nb/serde.h>
#include <net/link.h>

namespace net::rpc::dummy::error {
    template <nb::AsyncReadableWritable RW>
    class Executor {
        RequestContext<RW> ctx_;

      public:
        explicit Executor(RequestContext<RW> &&ctx, Result result) : ctx_{etl::move(ctx)} {
            ctx_.set_response_property(result, 0);
        }

        inline nb::Poll<void> execute(
            frame::FrameService &frame_service,
            routing::RoutingService<RW> &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::dummy::error
