#pragma once

#include "./accept.h"
#include "./node_delay.h"

namespace net::routing::worker {
    class PollAcceptTask {
        RoutingFrame frame_;

      public:
        explicit PollAcceptTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        inline nb::Poll<void> execute(AcceptWorker &accept_worker) {
            return accept_worker.poll_accept_frame(etl::move(frame_));
        }
    };

    class PollNodeDelayTask {
        RoutingFrame frame_;

      public:
        explicit PollNodeDelayTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        template <uint8_t N>
        inline nb::Poll<void> execute(
            NodeDelayWorker<N> &node_delay_worker,
            const node::LocalNodeService &lns,
            util::Time &time
        ) {
            return node_delay_worker.poll_push(lns, time, etl::move(frame_));
        }
    };

    template <uint8_t FRAME_ID_CACHE_SIZE>
    class ReceiveWorker {
        etl::optional<PollAcceptTask> accept_;
        etl::optional<PollNodeDelayTask> node_delay_;
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_;

      public:
        template <uint8_t N>
        inline void execute(
            AcceptWorker &accept_worker,
            NodeDelayWorker<N> &node_delay_worker,
            const node::LocalNodeService &lns,
            util::Time &time
        ) {
            if (accept_ && accept_->execute(accept_worker).is_ready()) {
                accept_.reset();
            }
            if (node_delay_ && node_delay_->execute(node_delay_worker, lns, time).is_ready()) {
                node_delay_.reset();
            }
        }

        // NodeId(self):        accept
        // ClusterId(self):     accept & broadcast
        // NodeId(not self):    discovery (-> unicast)
        // ClusterId(not lsef): dicsovery (-> unicast)
        inline nb::Poll<void>
        poll_accept_frame(const node::LocalNodeService &lns, RoutingFrame &&frame) {
            if (accept_ || node_delay_) {
                return nb::pending;
            }

            if (frame_id_cache_.insert_and_check_contains(frame.frame_id)) {
                return nb::ready();
            }

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            if (info.source.matches(frame.destination)) {
                accept_.emplace(frame.clone());
                if (!frame.destination.is_unicast()) {
                    node_delay_.emplace(etl::move(frame));
                }
            } else {
                node_delay_.emplace(etl::move(frame));
            }

            return nb::ready();
        }

        inline frame::FrameId generate_frame_id(util::Rand &rand) {
            return frame_id_cache_.generate(rand);
        }
    };
} // namespace net::routing::worker
