#pragma once

#include "./frame.h"
#include "./procedure.h"
#include "./request.h"
#include <net/routing.h>

namespace net::rpc {
    class RpcService {
        RequestReceiver receiver_;
        etl::optional<ProcedureExecutor> executor_;

      public:
        explicit RpcService(link::LinkService &link_service)
            : receiver_{routing::RoutingSocket{link_service.open(frame::ProtocolNumber::Rpc)}} {}

        void execute(
            frame::FrameService &fs,
            link::LinkService &ls,
            routing::RoutingService &rs,
            util::Time &time,
            util::Rand &rand
        ) {
            receiver_.execute(rs, time, rand);

            if (!executor_.has_value()) {
                auto opt_ctx = receiver_.poll_receive_frame(time);
                if (!opt_ctx.has_value()) {
                    return;
                }

                auto &ctx = opt_ctx.value();
                LOG_INFO("rpc: received request. Procedure: ", ctx.request().procedure());
                executor_.emplace(etl::move(ctx));
            }

            if (executor_->execute(fs, ls, rs, time, rand).is_ready()) {
                LOG_DEBUG("rpc: executor is ready");
                executor_.reset();
            }
        }
    };
}; // namespace net::rpc
