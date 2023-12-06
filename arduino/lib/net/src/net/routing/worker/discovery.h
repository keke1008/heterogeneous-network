#pragma once

#include "../service.h"
#include "./send_unicast.h"

namespace net::routing::worker {
    class DiscoveryTask {
        reactive::RouteDiscoverTask inner_task_;
        frame::FrameBufferReader reader_;
        etl::optional<node::NodeId> neighbor_id_;

      public:
        explicit DiscoveryTask(
            const node::NodeId &destination_id_,
            frame::FrameBufferReader &&reader_
        )
            : inner_task_{destination_id_},
              reader_{etl::move(reader_)} {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            SendUnicastWorker &send_unicast_worker,
            neighbor::NeighborService<RW> &neighbor_service,
            reactive::ReactiveService<RW> &reactive_service,
            RoutingService<RW> &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            const auto &self_id = POLL_UNWRAP_OR_RETURN(routing_service.poll_self_id());
            if (!neighbor_id_.has_value()) {
                neighbor_id_ = POLL_UNWRAP_OR_RETURN(inner_task_.execute(
                    neighbor_service, reactive_service, self_id.get(), routing_service.self_cost(),
                    time, rand
                ));

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
            neighbor::NeighborService<RW> &neighbor_service,
            reactive::ReactiveService<RW> &reactive_service,
            RoutingService<RW> &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!task_) {
                return;
            }

            auto poll = task_->execute(
                send_unicast_worker, neighbor_service, reactive_service, routing_service, time, rand
            );
            if (poll.is_ready()) {
                task_.reset();
            }
        }

        inline nb::Poll<void>
        poll_discover(const node::NodeId &destination_id, frame::FrameBufferReader &&reader) {
            if (task_) {
                return nb::pending;
            }

            task_.emplace(destination_id, etl::move(reader));
            return nb::ready();
        }
    };
} // namespace net::routing::worker
