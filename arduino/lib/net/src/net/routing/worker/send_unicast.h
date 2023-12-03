#pragma once

#include "../service.h"
#include <net/frame.h>
#include <net/neighbor.h>

namespace net::routing::worker {
    class SendUnicastTask {
        node::NodeId destination_id_;
        frame::FrameBufferReader reader_;
        etl::optional<nb::Promise<etl::expected<void, neighbor::SendError>>> promise_;

      public:
        explicit SendUnicastTask(
            const node::NodeId &destination_id,
            frame::FrameBufferReader &&reader,
            etl::optional<nb::Promise<etl::expected<void, neighbor::SendError>>> &&promise =
                etl::nullopt
        )
            : destination_id_{destination_id},
              reader_{etl::move(reader)},
              promise_{etl::move(promise)} {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            RoutingService<RW> &routing_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            auto result = POLL_UNWRAP_OR_RETURN(neighbor_socket.poll_send_frame(
                routing_service.neighbor_service_, destination_id_, etl::move(reader_)
            ));
            if (promise_) {
                promise_->set_value(etl::move(result));
            }
            return nb::ready();
        }
    };

    class SendUnicastWorker {
        etl::optional<SendUnicastTask> task_;

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

        inline nb::Poll<void>
        poll_send_frame(const node::NodeId &destination_id, frame::FrameBufferReader &&reader) {
            if (task_) {
                return nb::pending;
            }

            task_.emplace(destination_id, etl::move(reader));
            return nb::ready();
        }

        nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame_with_result(
            const node::NodeId &destination_id,
            frame::FrameBufferReader &&reader
        ) {
            if (task_) {
                return nb::pending;
            }

            auto [f, p] = nb::make_future_promise_pair<etl::expected<void, neighbor::SendError>>();
            task_.emplace(destination_id, etl::move(reader), etl::move(p));
            return etl::move(f);
        }
    };
} // namespace net::routing::worker
