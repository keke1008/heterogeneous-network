#pragma once

#include "./accept.h"
#include "./discovery.h"
#include "./send_broadcast.h"

namespace net::routing::worker {
    class PollAcceptTask {
        RoutingFrame frame_;

      public:
        explicit PollAcceptTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        inline nb::Poll<void> execute(AcceptWorker &accept_worker) {
            return accept_worker.poll_accept_frame(etl::move(frame_));
        }
    };

    class PollDiscoveryTask {
        discovery::Destination destination_;
        frame::FrameBufferReader reader_;

      public:
        explicit PollDiscoveryTask(
            const discovery::Destination &destination,
            frame::FrameBufferReader &&reader
        )
            : destination_{destination},
              reader_{etl::move(reader)} {}

        inline nb::Poll<void> execute(DiscoveryWorker &discovery_worker) {
            return discovery_worker.poll_discovery(destination_, etl::move(reader_));
        }
    };

    class PollSendBroadcastTask {
        frame::FrameBufferReader reader_;
        const etl::optional<node::NodeId> ignore_id_;

      public:
        explicit PollSendBroadcastTask(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        )
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id} {}

        inline nb::Poll<void> execute(SendBroadcastWorker &send_broadcast_worker) {
            return send_broadcast_worker.poll_send_broadcast_frame(etl::move(reader_), ignore_id_);
        }
    };

    template <uint8_t FRAME_ID_CACHE_SIZE>
    class ReceiveWorker {
        bool waiting_for_next_frame_ = true;

        etl::optional<PollAcceptTask> accept_;
        etl::optional<PollDiscoveryTask> discovery_;
        etl::optional<PollSendBroadcastTask> send_broadcast_;
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_;

      public:
        inline void execute(
            AcceptWorker &accept_worker,
            DiscoveryWorker &discovery_worker,
            SendBroadcastWorker &send_broadcast_worker
        ) {
            if (waiting_for_next_frame_) {
                return;
            }

            if (accept_ && accept_->execute(accept_worker).is_ready()) {
                accept_.reset();
            }
            if (discovery_ && discovery_->execute(discovery_worker).is_ready()) {
                discovery_.reset();
            }
            if (send_broadcast_ && send_broadcast_->execute(send_broadcast_worker).is_ready()) {
                send_broadcast_.reset();
            }

            waiting_for_next_frame_ = !accept_ && !discovery_ && !send_broadcast_;
        }

        // NodeId(self):        accept
        // ClusterId(self):     accept & broadcast
        // NodeId(not self):    discovery (-> unicast)
        // ClusterId(not lsef): dicsovery (-> unicast)
        inline nb::Poll<void>
        poll_accept_frame(const node::LocalNodeService &lns, RoutingFrame &&frame) {
            if (!waiting_for_next_frame_) {
                return nb::pending;
            }

            if (frame_id_cache_.contains(frame.frame_id)) {
                return nb::ready();
            }

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            if (frame.destination.matches(info.id, info.cluster_id)) {
                accept_.emplace(etl::move(frame));
                if (frame.destination.has_only_cluster_id()) {
                    send_broadcast_.emplace(etl::move(frame.payload), info.id);
                }
            } else {
                discovery_.emplace(frame.destination, etl::move(frame.payload));
            }

            return nb::ready();
        }
    };
} // namespace net::routing::worker
