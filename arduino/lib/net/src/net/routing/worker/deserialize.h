#pragma once

#include "./link_delay.h"

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

    class PollLinkDelayTask {
        RoutingFrame frame_;
        etl::optional<node::Cost> link_cost_;

      public:
        explicit PollLinkDelayTask(RoutingFrame &&frame) : frame_{etl::move(frame)} {}

        template <typename RW, uint8_t N>
        nb::Poll<void> execute(
            LinkDelayWorker<N> &link_delay,
            neighbor::NeighborService<RW> &ns,
            util::Time &time
        ) {
            if (!link_cost_.has_value()) {
                link_cost_ = ns.get_link_cost(frame_.source.node_id);
                if (!link_cost_.has_value()) {
                    return nb::ready();
                }
            }

            return link_delay.poll_push(time, *link_cost_, etl::move(frame_));
        }
    };

    class DeserializeWorker {
        etl::variant<etl::monostate, DeserializeTask, PollLinkDelayTask> task_;

      public:
        template <nb::AsyncReadableWritable RW, uint8_t N>
        void execute(
            LinkDelayWorker<N> &link_delay,
            neighbor::NeighborService<RW> &ns,
            neighbor::NeighborSocket<RW> &neighbor_socket,
            util::Time &time
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

                task_.emplace<PollLinkDelayTask>(etl::move(*opt_frame));
            }

            if (etl::holds_alternative<PollLinkDelayTask>(task_)) {
                auto &task = etl::get<PollLinkDelayTask>(task_);
                if (task.execute(link_delay, ns, time).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }
    };
} // namespace net::routing::worker
