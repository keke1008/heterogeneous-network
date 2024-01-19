#pragma once

#include "../../request.h"

namespace net::rpc::local::set_cluster_id {
    class Executor {
        RequestContext ctx_;
        node::AsyncOptionalClusterIdDeserializer param_{};

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
            node::OptionalClusterId cluster_id = param_.result();
            POLL_UNWRAP_OR_RETURN(lns.set_cluster_id(nts, cluster_id));

            ctx_.set_response_property(Result::Success, 0);
            return ctx_.poll_send_response(fs, lns, time, rand);
        }
    };
} // namespace net::rpc::local::set_cluster_id
