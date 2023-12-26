#pragma once

#include "./procedure.h"
#include "./request.h"
#include <net/routing.h>

namespace net::rpc {
    template <nb::AsyncReadableWritable RW>
    class RpcService {
        RequestReceiver<RW> receiver_;
        etl::optional<ProcedureExecutor<RW>> executor_;

      public:
        explicit RpcService(link::LinkService<RW> &link_service)
            : receiver_{routing::RoutingSocket<RW, FRAME_DELAY_POOL_SIZE>{
                  link_service.open(frame::ProtocolNumber::Rpc)
              }} {}

        void execute(
            frame::FrameService &fs,
            link::LinkService<RW> &ls,
            net::notification::NotificationService &nts,
            net::local::LocalNodeService &lns,
            net::neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            receiver_.execute(fs, lns, ns, ds, time, rand);

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

            if (executor_->execute(fs, ls, nts, lns, ns, time, rand).is_ready()) {
                executor_.reset();
            }
        }
    };
}; // namespace net::rpc
