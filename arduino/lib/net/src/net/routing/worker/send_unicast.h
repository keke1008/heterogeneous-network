#pragma once

#include "../service.h"

namespace net::routing::worker {
    class SendUnicastTask {
        node::NodeId gateway_id_;
        frame::FrameBufferReader reader_;
        etl::optional<nb::Promise<etl::expected<void, neighbor::SendError>>> promise_;

      public:
        explicit SendUnicastTask(
            const node::NodeId &gateway_id,
            frame::FrameBufferReader &&reader,
            etl::optional<nb::Promise<etl::expected<void, neighbor::SendError>>> &&promise =
                etl::nullopt
        )
            : gateway_id_{gateway_id},
              reader_{etl::move(reader)},
              promise_{etl::move(promise)} {}

        template <nb::AsyncReadableWritable RW>
        inline nb::Poll<void> execute(
            neighbor::NeighborService<RW> &neighbor_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            etl::expected<nb::Poll<void>, SendError> expected_poll_result =
                neighbor_socket.poll_send_frame(neighbor_service, gateway_id_, etl::move(reader_));
            if (!expected_poll_result.has_value()) {
                if (promise_) {
                    promise_->set_value(etl::unexpected<neighbor::SendError>{
                        expected_poll_result.error(),
                    });
                }
                return nb::ready();
            }

            POLL_UNWRAP_OR_RETURN(expected_poll_result.value());

            if (promise_) {
                promise_->set_value(etl::expected<void, neighbor::SendError>{});
            }
            return nb::ready();
        }
    };

    class SendUnicastWorker {
        etl::optional<SendUnicastTask> task_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            neighbor::NeighborService<RW> &neighbor_service,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            if (!task_) {
                return;
            }

            auto poll = task_->execute(neighbor_service, neighbor_socket);
            if (poll.is_ready()) {
                task_.reset();
            }
        }

        inline nb::Poll<void>
        poll_send_frame(const node::NodeId &gateway_id, frame::FrameBufferReader &&reader) {
            if (task_) {
                return nb::pending;
            }

            task_.emplace(gateway_id, etl::move(reader));
            return nb::ready();
        }

        nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame_with_result(
            const node::NodeId &gateway_id,
            frame::FrameBufferReader &&reader
        ) {
            if (task_) {
                return nb::pending;
            }

            auto [f, p] = nb::make_future_promise_pair<etl::expected<void, neighbor::SendError>>();
            task_.emplace(gateway_id, etl::move(reader), etl::move(p));
            return etl::move(f);
        }
    };
} // namespace net::routing::worker
