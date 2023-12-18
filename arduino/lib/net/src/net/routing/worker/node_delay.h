#pragma once

#include "../frame.h"
#include "./send.h"
#include <nb/time.h>
#include <net/frame.h>

namespace net::routing::worker {
    template <uint8_t MAX_FRAME_CAPACITY>
    class NodeDelayWorker {
        nb::DelayPool<RoutingFrame, MAX_FRAME_CAPACITY> delay_pool_;

      public:
        inline nb::Poll<void>
        poll_push(const node::LocalNodeService &lns, util::Time &time, RoutingFrame &&frame) {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            return delay_pool_.push(time, etl::move(frame), util::Duration(info.cost));
        }

        inline void
        execute(SendWorker &send_worker, const node::LocalNodeService &lns, util::Time &time) {
            auto seeker = delay_pool_.seek(time);
            while (auto &&entry = seeker.current()) {
                auto poll = send_worker.poll_repeat_frame(lns, etl::move(entry->get()));
                if (poll.is_ready()) {
                    seeker.remove_and_next();
                } else {
                    return;
                }
            }
        }
    };
} // namespace net::routing::worker
