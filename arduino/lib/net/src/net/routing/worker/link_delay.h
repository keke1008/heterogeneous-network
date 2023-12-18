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
            return delay_pool_.push(time, etl::move(frame), util::Duration(link_cost));
        }

        template <uint8_t N>
        inline void execute(
            ReceiveWorker<N> &receive_worker,
            const node::LocalNodeService &lns,
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
