#pragma once

#include "./accept.h"
#include "./send.h"

namespace net::routing::worker {
    class PollAcceptTask {
        RoutingFrame frame_;

      public:
        explicit PollAcceptTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        inline nb::Poll<void> execute(AcceptWorker &accept_worker) {
            return accept_worker.poll_accept_frame(etl::move(frame_));
        }
    };

    class PollRepeatTask {
        RoutingFrame frame_;

      public:
        explicit PollRepeatTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        inline nb::Poll<void> execute(SendWorker &send_worker, const node::LocalNodeService &lns) {
            return send_worker.poll_repeat_frame(
                lns, frame_.source, frame_.destination, etl::move(frame_.payload)
            );
        }
    };

    template <uint8_t FRAME_ID_CACHE_SIZE>
    class ReceiveWorker {
        etl::optional<PollAcceptTask> accept_;
        etl::optional<PollRepeatTask> send_;
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_;

      public:
        inline void execute(
            AcceptWorker &accept_worker,
            SendWorker &send_worker,
            const node::LocalNodeService &lns
        ) {
            if (accept_ && accept_->execute(accept_worker).is_ready()) {
                accept_.reset();
            }
            if (send_ && send_->execute(send_worker, lns).is_ready()) {
                send_.reset();
            }
        }

        // NodeId(self):        accept
        // ClusterId(self):     accept & broadcast
        // NodeId(not self):    discovery (-> unicast)
        // ClusterId(not lsef): dicsovery (-> unicast)
        inline nb::Poll<void>
        poll_accept_frame(const node::LocalNodeService &lns, RoutingFrame &&frame) {
            if (accept_ || send_) {
                return nb::pending;
            }

            if (frame_id_cache_.insert_and_check_contains(frame.frame_id)) {
                return nb::ready();
            }

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            if (info.source.matches(frame.destination)) {
                accept_.emplace(frame.clone());
                if (!frame.destination.is_unicast()) {
                    send_.emplace(etl::move(frame));
                }
            } else {
                send_.emplace(etl::move(frame));
            }

            return nb::ready();
        }

        inline frame::FrameId generate_frame_id(util::Rand &rand) {
            return frame_id_cache_.generate(rand);
        }
    };
} // namespace net::routing::worker
