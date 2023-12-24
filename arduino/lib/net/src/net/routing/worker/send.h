#pragma once

#include "./discovery.h"
#include "./send_broadcast.h"

namespace net::routing::worker {
    class PollDiscoveryTask {
        node::Destination destination_;
        frame::FrameBufferReader reader_;
        etl::optional<nb::Promise<SendResult>> promise_;

      public:
        explicit PollDiscoveryTask(
            const node::Destination &destination,
            frame::FrameBufferReader &&reader,
            etl::optional<nb::Promise<SendResult>> &&promise = etl::nullopt
        )
            : destination_{destination},
              reader_{etl::move(reader)},
              promise_{etl::move(promise)} {}

        inline nb::Poll<void> execute(DiscoveryWorker &discovery_worker) {
            return discovery_worker.poll_discovery(
                destination_, etl::move(reader_), etl::move(promise_)
            );
        }
    };

    class PollSendBroadcastTask {
        frame::FrameBufferReader reader_;
        const etl::optional<node::NodeId> ignore_id_;

      public:
        explicit PollSendBroadcastTask(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id
        )
            : reader_{etl::move(reader)},
              ignore_id_{ignore_id} {}

        inline nb::Poll<void> execute(SendBroadcastWorker &send_broadcast_worker) {
            return send_broadcast_worker.poll_send_broadcast_frame(etl::move(reader_), ignore_id_);
        }
    };

    class SendWorker {
        etl::variant<etl::monostate, PollDiscoveryTask, PollSendBroadcastTask> task_;

        inline bool is_waiting_for_next_frame() const {
            return etl::holds_alternative<etl::monostate>(task_);
        }

        inline void replace_task(
            const local::LocalNodeInfo &info,
            const node::Source &source,
            const node::Destination &destination,
            const frame::FrameBufferReader &reader,
            etl::optional<nb::Promise<SendResult>> &&promise
        ) {
            if (destination.is_unicast()) {
                task_.emplace<PollDiscoveryTask>(destination, reader.origin(), etl::move(promise));
            } else {
                task_.emplace<PollSendBroadcastTask>(reader.origin(), source.node_id);
                if (promise) {
                    promise->set_value(etl::expected<void, SendError>{});
                }
            }
        }

      public:
        inline nb::Poll<void>
        poll_repeat_frame(const local::LocalNodeService &lns, RoutingFrame &&frame) {
            if (!is_waiting_for_next_frame()) {
                return nb::pending;
            }

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            replace_task(info, frame.source, frame.destination, frame.payload, etl::nullopt);
            return nb::ready();
        }

        inline nb::Poll<nb::Future<SendResult>> poll_send_frame(
            const local::LocalNodeService &lns,
            const node::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            if (!is_waiting_for_next_frame()) {
                return nb::pending;
            }

            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            auto [f, p] = nb::make_future_promise_pair<SendResult>();
            replace_task(info, info.source, destination, reader, etl::move(p));
            return etl::move(f);
        }

        void
        execute(DiscoveryWorker &discovery_worker, SendBroadcastWorker &send_broadcast_worker) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                return;
            }

            auto poll = etl::visit(
                util::Visitor{
                    [&](etl::monostate &) { return nb::pending; },
                    [&](PollDiscoveryTask &task) { return task.execute(discovery_worker); },
                    [&](PollSendBroadcastTask &task) {
                        return task.execute(send_broadcast_worker);
                    },
                },
                task_
            );
            if (poll.is_ready()) {
                task_.emplace<etl::monostate>();
            }
        }
    };
} // namespace net::routing::worker
