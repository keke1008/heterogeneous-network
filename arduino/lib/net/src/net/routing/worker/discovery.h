#pragma once

#include "./send_unicast.h"

namespace net::routing::worker {
    class DiscoveryTask {
        discovery::DiscoveryTask discovery_task_;
        frame::FrameBufferReader reader_;
        etl::optional<node::NodeId> neighbor_id_;

      public:
        explicit DiscoveryTask(
            const discovery::Destination &destination,
            frame::FrameBufferReader &&reader
        )
            : discovery_task_{destination},
              reader_{etl::move(reader)} {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            SendUnicastWorker &send_unicast_worker,
            const node::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            if (!neighbor_id_.has_value()) {
                neighbor_id_ =
                    POLL_UNWRAP_OR_RETURN(discovery_task_.execute(lns, ns, ds, time, rand));

                if (!neighbor_id_.has_value()) {
                    return nb::ready();
                }
            }

            return send_unicast_worker.poll_send_frame(*neighbor_id_, etl::move(reader_));
        }
    };

    class DiscoveryWorker {
        etl::optional<DiscoveryTask> task_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            SendUnicastWorker &send_unicast_worker,
            const node::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!task_) {
                return;
            }

            auto poll = task_->execute(send_unicast_worker, lns, ns, ds, time, rand);
            if (poll.is_ready()) {
                task_.reset();
            }
        }

        inline nb::Poll<void> poll_discovery(
            const discovery::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            if (task_) {
                return nb::pending;
            }

            task_.emplace(destination, etl::move(reader));
            return nb::ready();
        }
    };
} // namespace net::routing::worker
