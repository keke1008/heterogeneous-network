#pragma once

#include "./constants.h"
#include "./task/create_repeat.h"
#include "./task/receive.h"
#include "./task/send.h"
#include <net/neighbor.h>

namespace net::routing::task {
    template <nb::AsyncReadableWritable RW, uint8_t FRAME_DELAY_POOL_SIZE>
    class TaskExecutor {
        etl::variant<etl::monostate, CreateRepeatFrameTask, SendFrameTask, ReceiveFrameTask> task_{
        };
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_{};
        nb::DelayPool<RoutingFrame, FRAME_DELAY_POOL_SIZE> delay_pool_{};
        etl::optional<RoutingFrame> accepted_frame_{};

        void receive_new_frame(
            neighbor::NeighborSocket<RW, FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time
        ) {
            FASSERT(etl::holds_alternative<etl::monostate>(task_));
            auto &&poll_frame = socket.poll_receive_frame(time);
            if (poll_frame.is_ready()) {
                task_.emplace<ReceiveFrameTask>(etl::move(poll_frame.unwrap()));
            }
        }

        void process_delayed_frame(const local::LocalNodeInfo &local, util::Time &time) {
            FASSERT(etl::holds_alternative<etl::monostate>(task_));
            if (accepted_frame_.has_value()) {
                return;
            }

            nb::Poll<RoutingFrame> &&poll_frame = delay_pool_.poll_pop_expired(time);
            if (poll_frame.is_pending()) {
                return;
            }

            auto &&frame = poll_frame.unwrap();
            if (local.source.matches(frame.destination)) {
                accepted_frame_.emplace(etl::move(frame));
                if (frame.destination.is_unicast()) {
                    return;
                }
            }

            task_.emplace<CreateRepeatFrameTask>(etl::move(frame), local.source.node_id);
        }

        void send_frame(
            node::Destination destination,
            frame::FrameBufferReader &&reader,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            FASSERT(etl::holds_alternative<etl::monostate>(task_));
            if (!local.source.matches(destination)) {
                task_ =
                    SendFrameTask::unicast(destination, reader.make_initial_clone(), etl::nullopt);
                return;
            }

            if (!destination.is_unicast()) {
                task_ = SendFrameTask::broadcast(
                    reader.make_initial_clone(), etl::nullopt, etl::nullopt
                );
            }
        }

      public:
        void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            neighbor::NeighborSocket<RW, FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time,
            util::Rand &rand

        ) {
            const auto &poll_local = lns.poll_info();
            if (poll_local.is_pending()) {
                return;
            }
            const auto &local = poll_local.unwrap();

            if (etl::holds_alternative<etl::monostate>(task_)) {
                process_delayed_frame(local, time);
                if (etl::holds_alternative<etl::monostate>(task_)) {
                    receive_new_frame(socket, time);
                }
            }

            if (etl::holds_alternative<CreateRepeatFrameTask>(task_)) {
                auto &task = etl::get<CreateRepeatFrameTask>(task_);
                auto &&poll_reader = task.execute(fs, lns, socket);
                if (poll_reader.is_pending()) {
                    return;
                }

                auto destination = etl::get<CreateRepeatFrameTask>(task_).destination();
                task_.emplace<etl::monostate>();
                send_frame(destination, etl::move(poll_reader.unwrap()), local, time);
            }

            if (etl::holds_alternative<ReceiveFrameTask>(task_)) {
                auto &receive_task = etl::get<ReceiveFrameTask>(task_);
                auto poll_result =
                    receive_task.execute(ns, delay_pool_, frame_id_cache_, local, time);
                if (poll_result.is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &send_task = etl::get<SendFrameTask>(task_);
                auto poll_result = send_task.execute(lns, ns, ds, socket, time, rand);
                if (poll_result.is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }
        }

        inline nb::Poll<nb::Future<SendResult>> poll_send_frame(
            const local::LocalNodeService &lns,
            const node::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            if (!etl::holds_alternative<etl::monostate>(task_)) {
                return nb::pending;
            }

            auto [f, p] = nb::make_future_promise_pair<SendResult>();
            task_ = destination.is_unicast()
                ? SendFrameTask::unicast(destination, etl::move(reader), etl::move(p))
                : SendFrameTask::broadcast(etl::move(reader), etl::nullopt, etl::move(p));
            return etl::move(f);
        }

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            if (accepted_frame_.has_value()) {
                auto frame = etl::move(*accepted_frame_);
                accepted_frame_.reset();
                return frame;
            } else {
                return nb::pending;
            }
        }

        inline frame::FrameId generate_frame_id(util::Rand &rand) {
            return frame_id_cache_.generate(rand);
        }
    };
} // namespace net::routing::task
