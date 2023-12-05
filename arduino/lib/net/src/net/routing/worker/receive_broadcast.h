#pragma once

#include "./accept.h"
#include "./send_broadcast.h"

namespace net::routing::worker {
    class ReceiveBroadcastTask {
        bool done_accept_ = true;
        BroadcastRoutingFrame frame_;

        bool done_send_ = true;
        frame::FrameBufferReader reader_;
        node::NodeId ignore_id_;

      public:
        explicit ReceiveBroadcastTask(BroadcastRoutingFrame &&frame)
            : frame_{etl::move(frame)},
              reader_{frame_.payload.origin()},
              ignore_id_{frame.header.sender_id} {}

        nb::Poll<void>
        execute(AcceptWorker &accept_worker, SendBroadcastWorker &send_broadcast_worker) {
            if (!done_accept_) {
                done_accept_ =
                    accept_worker.poll_accept_broadcast_frame(etl::move(frame_)).is_ready();
            }

            if (!done_send_) {
                done_send_ =
                    send_broadcast_worker.poll_send_broadcast_frame(etl::move(reader_), ignore_id_)
                        .is_ready();
            }

            if (done_accept_ && done_send_) {
                return nb::ready();
            }

            return nb::pending;
        }
    };

    class Pending {
        BroadcastRoutingFrame frame_;

      public:
        explicit Pending(BroadcastRoutingFrame &&frame) : frame_{etl::move(frame)} {}

        BroadcastRoutingFrame &&frame() {
            return etl::move(frame_);
        }
    };

    template <uint8_t FRAME_ID_CACHE_SIZE>
    class ReceiveBroadcastWorker {
        etl::variant<etl::monostate, Pending, ReceiveBroadcastTask> task_;
        FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_;

      public:
        inline FrameId generate_frame_id() {
            return frame_id_cache_.generate();
        }

        void execute(AcceptWorker &accept_worker, SendBroadcastWorker &send_broadcast_worker) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                return;
            }

            if (etl::holds_alternative<Pending>(task_)) {
                auto &&frame = etl::get<Pending>(task_).frame();
                bool contains = frame_id_cache_.contains(frame.header.frame_id);
                frame_id_cache_.add(frame.header.frame_id);
                if (contains) {
                    task_.emplace<etl::monostate>();
                    return;
                }

                task_.emplace<ReceiveBroadcastTask>(etl::move(frame));
            }

            auto &task = etl::get<ReceiveBroadcastTask>(task_);
            if (task.execute(accept_worker, send_broadcast_worker).is_ready()) {
                task_.emplace<etl::monostate>();
            }
        }

        inline nb::Poll<void> poll_receive_frame(BroadcastRoutingFrame &&frame) {
            if (!etl::holds_alternative<etl::monostate>(task_)) {
                return nb::pending;
            }

            task_.emplace<Pending>(etl::move(frame));
            return nb::ready();
        }
    };
} // namespace net::routing::worker
