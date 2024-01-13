#pragma once

#include "./constants.h"
#include "./event.h"
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
        etl::optional<RoutingFrame> accepted_frame_{};

        void send_frame(
            node::Destination destination,
            frame::FrameBufferReader &&reader,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            FASSERT(etl::holds_alternative<etl::monostate>(task_));
            auto unicast = [&]() {
                task_ = SendFrameTask::unicast(destination, etl::move(reader), etl::nullopt);
            };
            auto broadcast = [&]() {
                task_ = SendFrameTask::broadcast(etl::move(reader), etl::nullopt, etl::nullopt);
            };

            if (destination.is_unicast()) {
                unicast();
                return;
            }

            if (destination.is_broadcast()) {
                broadcast();
                return;
            }

            // マルチキャストの場合
            if (local.source.matches(destination)) {
                // 宛先に自分が含まれている場合はブロードキャストする
                broadcast();
            } else {
                // マルチキャストの宛先へ中継する
                unicast();
            }
        }

      public:
        RoutingSocketEvent execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            neighbor::NeighborSocket<RW, FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time,
            util::Rand &rand

        ) {
            RoutingSocketEvent result = RoutingSocketEvent::empty();

            const auto &poll_local = lns.poll_info();
            if (poll_local.is_pending()) {
                return result;
            }
            const auto &local = poll_local.unwrap();

            if (etl::holds_alternative<etl::monostate>(task_)) {
                nb::Poll<neighbor::ReceivedNeighborFrame> &&poll_frame =
                    socket.poll_receive_frame(time);
                if (poll_frame.is_ready()) {
                    task_.emplace<ReceiveFrameTask>(etl::move(poll_frame.unwrap()));
                }
            }

            if (etl::holds_alternative<CreateRepeatFrameTask>(task_)) {
                auto &task = etl::get<CreateRepeatFrameTask>(task_);
                auto &&poll_opt_reader = task.execute(fs, lns, socket);
                if (poll_opt_reader.is_pending()) {
                    return result;
                }

                auto &&opt_reader = poll_opt_reader.unwrap();
                if (!opt_reader.has_value()) {
                    task_.emplace<etl::monostate>();
                    return result;
                }

                auto destination = etl::get<CreateRepeatFrameTask>(task_).destination();
                task_.emplace<etl::monostate>();
                send_frame(destination, etl::move(opt_reader.value()), local, time);
            }

            if (etl::holds_alternative<ReceiveFrameTask>(task_)) {
                auto &task = etl::get<ReceiveFrameTask>(task_);
                if (task.execute(ns, frame_id_cache_, local, time).is_pending()) {
                    return result;
                }

                auto &opt_frame = task.result();
                if (!opt_frame.has_value()) {
                    task_.emplace<etl::monostate>();
                    return result;
                }

                result.set_frame_received();

                // すでに受信済みのフレームがある場合は、今受信したフレームを破棄する
                if (accepted_frame_.has_value()) {
                    task_.emplace<etl::monostate>();
                    return result;
                }

                auto frame = etl ::move(opt_frame.value());
                task_.emplace<etl::monostate>();
                if (local.source.matches(frame.destination)) {
                    accepted_frame_.emplace(frame.clone());

                    if (frame.destination.is_unicast()) {
                        return result;
                    }
                }

                task_.emplace<CreateRepeatFrameTask>(etl::move(frame), local.source.node_id);
            }

            if (etl::holds_alternative<SendFrameTask>(task_)) {
                auto &send_task = etl::get<SendFrameTask>(task_);
                auto poll_result = send_task.execute(lns, ns, ds, socket, time, rand);
                if (poll_result.is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            return result;
        }

        inline nb::Poll<nb::Future<SendResult>>
        poll_send_frame(const node::Destination &destination, frame::FrameBufferReader &&reader) {
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
