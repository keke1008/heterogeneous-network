#pragma once

#include "../service.h"

namespace net::routing::worker {
    class SendBroadcastTask {
        frame::FrameBufferReader reader_;
        etl::optional<node::NodeId> ignore_id_;

      public:
        explicit SendBroadcastTask(frame::FrameBufferReader &&reader, const node::NodeId &ignore_id)
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id} {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            RoutingService<RW> &routing_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            POLL_UNWRAP_OR_RETURN(neighbor_socket.poll_send_broadcast_frame(
                routing_service.neighbor_service_, etl::move(reader_), ignore_id_
            ));
            return nb::ready();
        }
    };

    class SendBroadcastWorker {
        etl::optional<SendBroadcastTask> task_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            RoutingService<RW> &routing_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            if (!task_) {
                return;
            }

            auto poll = task_->execute(routing_service, neighbor_socket);
            if (poll.is_ready()) {
                task_.reset();
            }
        }

        inline nb::Poll<void> poll_send_broadcast_frame(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        ) {
            if (task_) {
                return nb::pending;
            }

            task_.emplace(etl::move(reader), ignore_id);
            return nb::ready();
        }
    };
} // namespace net::routing::worker
