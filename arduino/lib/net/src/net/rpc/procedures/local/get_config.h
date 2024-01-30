#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::local::get_config {
    class Executor {
        RequestContext ctx_;
        etl::optional<net::local::AsyncLocalNodeConfigSerializer> result_;

      public:
        explicit inline Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            const net::local::LocalNodeService &lns,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_ready_to_send_response()) {
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            if (!result_.has_value()) {
                result_.emplace(lns.config());
                ctx_.set_response_property(Result::Success, result_->serialized_length());
            }

            auto writer = POLL_UNWRAP_OR_RETURN(ctx_.poll_response_writer(fs, lns, rand));
            writer.get().serialize_all_at_once(*result_);
            return ctx_.poll_send_response(fs, lns, time, rand);
        }
    };
} // namespace net::rpc::local::get_config
