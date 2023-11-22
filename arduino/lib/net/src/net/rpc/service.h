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
            if (!executor_.has_value()) {
                auto opt_ctx = receiver_.execute(time);
                if (!opt_ctx.has_value()) {
                    return;
                }
                executor_.emplace(etl::move(opt_ctx.value()));
            }

            if (executor_->execute(fs, ls, rs, time, rand).is_ready()) {
                executor_.reset();
            }
        }
    };
}; // namespace net::rpc
