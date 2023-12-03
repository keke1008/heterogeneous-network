#pragma once

#include "./receive_broadcast.h"
#include "./receive_unicast.h"

namespace net::routing::worker {
    class DeserializeTask {
        link::LinkFrame frame_;
        AsyncRoutingFrameHeaderDeserializer header_deserializer_{};

      public:
        explicit DeserializeTask(link::LinkFrame &&frame) : frame_{etl::move(frame)} {}

        using Result = etl::variant<etl::monostate, BroadcastRoutingFrame, UnicastRoutingFrame>;

        nb::Poll<Result> execute() {
            auto result = POLL_UNWRAP_OR_RETURN(frame_.reader.deserialize(header_deserializer_));
            if (result != nb::DeserializeResult::Ok) {
                return Result{};
            }

            const auto &header = header_deserializer_.result();
            if (etl::holds_alternative<BroadcastRoutingFrameHeader>(header)) {
                return Result{BroadcastRoutingFrame{
                    .header = etl::get<BroadcastRoutingFrameHeader>(header),
                    .payload = etl::move(frame_.reader),
                }};
            } else {
                return Result{UnicastRoutingFrame{
                    .header = etl::get<UnicastRoutingFrameHeader>(header),
                    .payload = etl::move(frame_.reader),
                }};
            }
        }
    };

    class PollReceiveFrameTask {
        etl::variant<BroadcastRoutingFrame, UnicastRoutingFrame> frame_;

      public:
        explicit PollReceiveFrameTask(BroadcastRoutingFrame &&frame) : frame_{etl::move(frame)} {}

        explicit PollReceiveFrameTask(UnicastRoutingFrame &&frame) : frame_{etl::move(frame)} {}

        template <uint8_t N>
        nb::Poll<void> execute(
            ReceiveBroadcastWorker<N> &receive_broadcast_worker,
            ReceiveUnicastWorker &receive_unicast_worker
        ) {
            if (etl::holds_alternative<BroadcastRoutingFrame>(frame_)) {
                return receive_broadcast_worker.poll_receive_frame(
                    etl::move(etl::get<BroadcastRoutingFrame>(frame_))
                );
            } else {
                return receive_unicast_worker.poll_receive_frame(
                    etl::move(etl::get<UnicastRoutingFrame>(frame_))
                );
            }
        }
    };

    class DeserializeWorker {
        etl::variant<etl::monostate, DeserializeTask, PollReceiveFrameTask> task_;

      public:
        template <nb::AsyncReadableWritable RW, uint8_t N>
        void execute(
            ReceiveBroadcastWorker<N> &receive_broadcast_worker,
            ReceiveUnicastWorker &receive_unicast_worker,
            neighbor::NeighborSocket<RW> &neighbor_socket
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                const auto &poll_frame = neighbor_socket.poll_receive_frame();
                if (poll_frame.is_pending()) {
                    return;
                }

                task_.emplace<DeserializeTask>(etl::move(poll_frame).unwrap());
            }

            if (etl::holds_alternative<DeserializeTask>(task_)) {
                const auto &result = etl::get<DeserializeTask>(task_).execute();
                if (result.is_pending()) {
                    return;
                }

                task_.emplace<PollReceiveFrameTask>(etl::move(result).unwrap());
            }

            if (etl::holds_alternative<PollReceiveFrameTask>(task_)) {
                auto &task = etl::get<PollReceiveFrameTask>(task_);
                if (task.execute(receive_broadcast_worker, receive_unicast_worker).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }
    };
} // namespace net::routing::worker
