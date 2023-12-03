#pragma once

#include "./accept.h"
#include "./discovery.h"

namespace net::routing::worker {
    class ReceiveUnicastTask {
        enum class Operation { Accept, Discovery, Unkown } operation_ = Operation::Unkown;
        UnicastRoutingFrame frame_;

      public:
        explicit ReceiveUnicastTask(UnicastRoutingFrame &&frame) : frame_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> execute(
            AcceptWorker &accept_worker,
            DiscoveryWorker &discovery_worker,
            RoutingService<RW> &routing_service
        ) {
            if (operation_ == Operation::Unkown) {
                const auto &local_id = POLL_UNWRAP_OR_RETURN(routing_service.poll_self_id());
                operation_ =
                    frame_.header.target_id == local_id ? Operation::Accept : Operation::Discovery;
            }

            if (operation_ == Operation::Accept) {
                return accept_worker.poll_accept_unicast_frame(etl::move(frame_));
            } else {
                return discovery_worker.poll_discover(
                    frame_.header.target_id, frame_.payload.origin()
                );
            }
        }
    };

    class ReceiveUnicastWorker {
        etl::optional<ReceiveUnicastTask> task_;

      public:
        template <nb::AsyncReadableWritable RW>
        inline void execute(
            AcceptWorker &accept_worker,
            DiscoveryWorker &discovery_worker,
            RoutingService<RW> &routing_service
        ) {
            if (!task_) {
                return;
            }

            if (task_->execute(accept_worker, discovery_worker, routing_service).is_ready()) {
                task_.reset();
            }
        }

        inline nb::Poll<void> poll_receive_frame(UnicastRoutingFrame &&frame) {
            if (task_) {
                return nb::pending;
            }

            task_.emplace(etl::move(frame));
            return nb::ready();
        }
    };
} // namespace net::routing::worker
