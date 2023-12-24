#pragma once

#include "./cache.h"
#include "./frame.h"
#include "./task/receive.h"
#include "./task/send.h"
#include <etl/variant.h>
#include <net/neighbor.h>

namespace net::discovery {
    struct DiscoveryEvent {
        node::Destination destination;
        node::NodeId gateway_id;
        TotalCost total_cost;

        DiscoveryEvent(const DiscoveryFrame &frame, TotalCost total_cost)
            : destination{frame.target},
              gateway_id{frame.sender.node_id},
              total_cost{total_cost} {}
    };

    template <nb::AsyncReadableWritable RW>
    class TaskExecutor {
        neighbor::NeighborSocket<RW> socket_;
        task::FrameDelayPool delay_pool_{};
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_{};
        etl::variant<etl::monostate, task::ReceiveFrameTask, task::SendFrameTask> task_{};

      public:
        explicit TaskExecutor(neighbor::NeighborSocket<RW> &&socket) : socket_{etl::move(socket)} {}

        etl::optional<DiscoveryEvent> execute(
            frame::FrameService &fs,
            neighbor::NeighborService<RW> &ns,
            DiscoveryCache &discovery_cache,
            const local::LocalNodeInfo &local,
            util::Time &time,
            util::Rand &rand
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto &&poll_frame = socket_.poll_receive_frame();
                if (poll_frame.is_ready()) {
                    task_.emplace<task::ReceiveFrameTask>(etl::move(poll_frame.unwrap()));
                } else {
                    auto &&poll_frame = delay_pool_.poll_pop_expired(time);
                    if (poll_frame.is_pending()) {
                        return etl::nullopt;
                    }

                    // 既にキャッシュにある（受信済み）場合は無視する
                    auto &&frame = poll_frame.unwrap();
                    if (frame_id_cache_.insert_and_check_contains(frame.frame_id)) {
                        return etl::nullopt;
                    }

                    // 送信元とのリンクコストが不明（送信元がNeighborでない）場合は無視する
                    auto opt_link_cost = ns.get_link_cost(frame.sender.node_id);
                    if (!opt_link_cost) {
                        return etl::nullopt;
                    }
                    auto total_cost = frame.calculate_total_cost(*opt_link_cost, local.cost);

                    // キャッシュに追加
                    discovery_cache.update_by_received_frame(frame, total_cost, time);

                    if (local.source.matches(frame.destination())) { // 自分自身が探索対象の場合
                        if (frame.type == DiscoveryFrameType::Request) {
                            // Requestであれば探索元に返信する
                            task_ = task::SendFrameTask::reply(frame, local, frame_id_cache_, rand);
                        } else {
                            // Replyであれば探索結果を返す
                            return DiscoveryEvent{frame, total_cost};
                        }
                    } else { // 自分自身が探索対象でない場合
                        auto opt_cache = discovery_cache.get(frame.destination());
                        if (!opt_cache) {
                            // 探索対象がキャッシュにない場合，ブロードキャストする
                            task_ = task::SendFrameTask::repeat_broadcast(frame, local, total_cost);
                        } else if (frame.type == DiscoveryFrameType::Request) {
                            // 探索対象がキャッシュにある場合，キャッシュからゲートウェイを取得して返信する
                            task_ = task::SendFrameTask::reply_by_cache(
                                frame, local, frame_id_cache_, opt_cache->get(), rand
                            );
                        } else {
                            // Replyであれば中継する
                            task_ = task::SendFrameTask::repeat_unicast(
                                frame, local, total_cost, opt_cache->get().gateway_id
                            );
                        }
                    }
                }
            }

            if (etl::holds_alternative<task::ReceiveFrameTask>(task_)) {
                auto &task = etl::get<task::ReceiveFrameTask>(task_);
                if (task.execute(ns, delay_pool_, frame_id_cache_, local, time).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            if (etl::holds_alternative<task::SendFrameTask>(task_)) {
                auto &task = etl::get<task::SendFrameTask>(task_);
                if (task.execute(fs, ns, socket_).is_ready()) {
                    task_.emplace<etl::monostate>();
                }
            }

            return etl::nullopt;
        }

        nb::Poll<void> request_send_discovery_frame(
            const node::Destination &destination,
            const local::LocalNodeInfo &local,
            util::Rand &rand
        ) {
            if (etl::holds_alternative<etl::monostate>(task_)) {
                task_ = task::SendFrameTask::request(destination, local, frame_id_cache_, rand);
                return nb::ready();
            } else {
                return nb::pending;
            }
        }
    };
} // namespace net::discovery
