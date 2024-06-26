#pragma once

#include "./procedure.h"
#include "./request.h"
#include <net/routing.h>

namespace net::rpc {
    class RpcService {
        RequestReceiver receiver_;
        etl::optional<ProcedureExecutor> executor_;

      public:
        explicit RpcService(link::LinkService &link_service)
            : receiver_{routing::RoutingSocket<FRAME_DELAY_POOL_SIZE>{
                  link_service.open(frame::ProtocolNumber::Rpc), SOCKET_CONFIG
              }} {}

        void execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            link::LinkService &ls,
            net::notification::NotificationService &nts,
            net::local::LocalNodeService &lns,
            net::neighbor::NeighborService &ns,
            discovery::DiscoveryService &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            receiver_.execute(fs, ms, lns, ns, ds, time, rand);

            if (!executor_.has_value()) {
                auto opt_ctx = receiver_.poll_receive_frame(time);
                if (!opt_ctx.has_value()) {
                    return;
                }

                auto &ctx = opt_ctx.value();
                LOG_INFO(
                    FLASH_STRING("rpc: received request. Procedure: "), ctx.request().procedure()
                );
                executor_.emplace(etl::move(ctx));
            }

            if (executor_->execute(fs, ms, ls, nts, lns, ns, time, rand).is_ready()) {
                executor_.reset();
            }
        }
    };
}; // namespace net::rpc
