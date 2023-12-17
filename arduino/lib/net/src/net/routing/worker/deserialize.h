#pragma once

#include "./receive.h"

namespace net::routing::worker {
    class DeserializeTask {
        link::LinkFrame frame_;
        AsyncRoutingFrameDeserializer deserializer_;

      public:
        explicit DeserializeTask(link::LinkFrame &&frame) : frame_{etl::move(frame)} {}

        nb::Poll<etl::optional<RoutingFrame>> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(deserializer_));
            return result == nb::DeserializeResult::Ok
                ? deserializer_.as_frame(etl::move(frame_.reader))
                : etl::optional<RoutingFrame>{};
        }
    };

    class PollReceiveFrameTask {
        RoutingFrame frame_;

      public:
        explicit PollReceiveFrameTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        template <uint8_t N>
        nb::Poll<void>
        execute(ReceiveWorker<N> &receive_worker, const node::LocalNodeService &lns) {
            return receive_worker.poll_accept_frame(lns, etl::move(frame_));
        }
    };

    class DeserializeWorker {
        etl::variant<etl::monostate, DeserializeTask, PollReceiveFrameTask> task_;

      public:
        template <nb::AsyncReadableWritable RW, uint8_t N>
        void execute(
            ReceiveWorker<N> &receive_worker,
            const node::LocalNodeService &lns,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                nb::Poll<link::LinkFrame> &&poll_frame = neighbor_socket.poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return;
                }
                task_.emplace<DeserializeTask>(etl::move(poll_frame.unwrap()));
            }

            if (etl::holds_alternative<DeserializeTask>(task_)) {
                auto &&poll_opt_frame = etl::get<DeserializeTask>(task_).execute();
                if (poll_opt_frame.is_pending()) {
                    return;
                }

                auto &&opt_frame = poll_opt_frame.unwrap();
                if (!opt_frame) {
                    task_.emplace<etl::monostate>();
                    return;
                }

                task_.emplace<PollReceiveFrameTask>(etl::move(*opt_frame));
            }

            if (etl::holds_alternative<PollReceiveFrameTask>(task_)) {
                auto &task = etl::get<PollReceiveFrameTask>(task_);
                if (task.execute(receive_worker, lns).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }
    };
} // namespace net::routing::worker
