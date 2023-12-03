#pragma once

#include "../service.h"
#include "./send_unicast.h"
#include <net/frame.h>
#include <net/neighbor.h>

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
            RoutingService<RW> &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!routing_service.self_id_) {
                return nb::pending;
            }

            if (!neighbor_id_.has_value()) {
                neighbor_id_ = POLL_UNWRAP_OR_RETURN(inner_task_.execute(
                    routing_service.reactive_service_, *routing_service.self_id_,
                    routing_service.self_cost_, time, rand
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
            RoutingService<RW> &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!task_) {
                return;
            }

            auto poll = task_->execute(send_unicast_worker, routing_service, time, rand);
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
