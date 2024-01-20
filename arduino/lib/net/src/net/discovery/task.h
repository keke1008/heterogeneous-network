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

        DiscoveryEvent(const ReceivedDiscoveryFrame &frame, TotalCost total_cost)
            : destination{frame.target},
              gateway_id{frame.previousHop},
              total_cost{total_cost} {}
    };

    class TaskExecutor {
        neighbor::NeighborSocket<FRAME_DELAY_POOL_SIZE> socket_;
        frame::FrameIdCache<FRAME_ID_CACHE_SIZE> frame_id_cache_{};
        etl::variant<etl::monostate, task::ReceiveFrameTask, task::SendFrameTask> task_{};

      public:
        explicit TaskExecutor(neighbor::NeighborSocket<FRAME_DELAY_POOL_SIZE> &&socket)
            : socket_{etl::move(socket)} {}

        etl::optional<DiscoveryEvent> execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            const local::LocalNodeService &lns,
            neighbor::NeighborService &ns,
            DiscoveryCache &discovery_cache,
            const local::LocalNodeInfo &local,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(ms, lns, ns, time);

            if (etl::holds_alternative<etl::monostate>(task_)) {
                auto &&poll_frame = socket_.poll_receive_frame(time);
                if (poll_frame.is_ready()) {
                    task_.emplace<task::ReceiveFrameTask>(etl::move(poll_frame.unwrap()));
                }
            }

            if (etl::holds_alternative<task::ReceiveFrameTask>(task_)) {
                nb::Poll<etl::optional<ReceivedDiscoveryFrame>> &&poll_opt_frame =
                    etl::get<task::ReceiveFrameTask>(task_).execute(
                        ns, frame_id_cache_, local, time
                    );
                if (poll_opt_frame.is_pending()) {
                    return etl::nullopt;
                }
                task_.emplace<etl::monostate>();

                auto &&opt_frame = poll_opt_frame.unwrap();
                if (!opt_frame) {
                    return etl::nullopt;
                }
                auto &&frame = opt_frame.value();

                auto opt_link_cost = ns.get_link_cost(frame.previousHop);
                FASSERT(opt_link_cost.has_value());
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

            if (etl::holds_alternative<task::SendFrameTask>(task_)) {
                auto &task = etl::get<task::SendFrameTask>(task_);
                if (task.execute(fs, lns, ns, socket_).is_ready()) {
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
