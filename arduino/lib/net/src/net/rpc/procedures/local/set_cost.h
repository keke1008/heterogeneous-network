#pragma once

#include "../../request.h"

namespace net::rpc::local::set_cost {
    class Executor {
        RequestContext ctx_;
        node::AsyncCostDeserializer param_{};

      public:
        explicit Executor(RequestContext ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            notification::NotificationService &nts,
            net::local::LocalNodeService &lns,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(param_));
            node::Cost cost = param_.result();
            POLL_UNWRAP_OR_RETURN(lns.set_cost(nts, cost));

            ctx_.set_response_property(Result::Success, 0);
            return ctx_.poll_send_response(fs, lns, time, rand);
        }
    };
} // namespace net::rpc::local::set_cost
