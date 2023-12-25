#pragma once

#include "../frame.h"
#include "./receive.h"
#include <nb/time.h>
#include <net/frame.h>

namespace net::routing::worker {
    template <uint8_t MAX_FRAME_CAPACITY>
    class LinkDelayWorker {
        nb::DelayPool<RoutingFrame, MAX_FRAME_CAPACITY> delay_pool_;

      public:
        inline nb::Poll<void>
        poll_push(util::Time &time, node::Cost link_cost, RoutingFrame &&frame) {
            POLL_UNWRAP_OR_RETURN(delay_pool_.poll_pushable());
            delay_pool_.push(etl::move(frame), util::Duration(link_cost), time);
            return nb::ready();
        }

        template <uint8_t N>
        inline void execute(
            ReceiveWorker<N> &receive_worker,
            const local::LocalNodeService &lns,
            util::Time &time
        ) {
            auto seeker = delay_pool_.seek(time);
            while (auto &&entry = seeker.current()) {
                auto poll = receive_worker.poll_accept_frame(lns, etl::move(entry->get()));
                if (poll.is_ready()) {
                    seeker.remove_and_next();
                } else {
                    return;
                }
            }
        }
    };
} // namespace net::routing::worker
